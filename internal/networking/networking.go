package networking

import (
	"fmt"
	"io/ioutil"
	"log"
	"net"
	"net/url"
	"strings"
	"time"

	"github.com/gladiusio/gladius-masternode/internal/networking/state"
	"github.com/spf13/viper"
	"github.com/valyala/fasthttp"
)

// StartProxy - Start a proxy server
func StartProxy() {
	fmt.Println("Starting...")

	fmt.Println("Loading route files")
	// read the whole file at once
	loaderHTML, err := ioutil.ReadFile("./html/loader_v2.html")
	if err != nil {
		panic(err)
	}

	// TODO: This needs to be a thread safe mapping that is loaded from the controld continually.
	hosts := make(map[string]string) // websites we are protecting/hosting for
	noCacheRoutes := make(map[string]map[string]bool)
	cachedRoutes := make(map[string]map[string]bool)
	expectedHash := make(map[string]map[string]string)

	// Define accepted hosts
	// TODO: will come from controld eventually (from p2p network)
	hosts["demo.gladius.io"] = "http://172.217.7.228"
	cachedRoutes["demo.gladius.io"] = make(map[string]bool)
	cachedRoutes["demo.gladius.io"]["/"] = true
	cachedRoutes["demo.gladius.io"]["/anotherroute"] = true

	noCacheRoutes["demo.gladius.io"] = make(map[string]bool)
	noCacheRoutes["demo.gladius.io"]["/api/"] = true

	expectedHash["demo.gladius.io"] = make(map[string]string)
	expectedHash["demo.gladius.io"]["/"] = "8476da67667d0c127963bf46c3b637935961014ebe155812d6fc7d64a4a37c41"
	expectedHash["demo.gladius.io"]["/anotherroute"] = "6F9ECF8D1FAD1D2B8FBF2DA3E2571AEC4267A7018DF0DBDE8889D875FBDE8D3F"

	// Create new network state object to keep track of edge nodes
	netState := state.NewNetworkState()

	go fasthttp.ListenAndServe(":8081", requestBuilder(hosts, cachedRoutes, noCacheRoutes, expectedHash, string(loaderHTML), netState))

	if viper.GetString("MININET_CONFIG") == "" {
		// Update network state every 30 seconds
		ticker := time.NewTicker(time.Second * 30)
		defer ticker.Stop()
		for {
			select {
			case <-ticker.C:
				netState.RefreshActiveNodes()
			}
		}
	}
}

func requestBuilder(hosts map[string]string, cachedRoutes, noCacheRoutes map[string]map[string]bool,
	expectedHash map[string]map[string]string, loaderHTML string, networkState *state.NetworkState) func(ctx *fasthttp.RequestCtx) {
	// The actual serving function
	return func(ctx *fasthttp.RequestCtx) {
		host := string(ctx.Host()[:])
		// If this host isn't recognized, break out
		if hosts[host] == "" {
			ctx.Error("Unsupported Host", fasthttp.StatusBadRequest)
			return
		}

		u, err := url.Parse(string(ctx.RequestURI()[:]))
		if err != nil {
			log.Fatal(err)
		}
		path := u.RequestURI()

		if cachedRoutes[host][path] { // The route is cached, return link to bundle
			ip := ctx.RemoteIP().String()

			// Determine which content node will serve the assets for this request
			contentNode, nodeErr := chooseContentNode(ip, networkState)

			if nodeErr != nil {
				proxyRequest(ctx, hosts[host]+path)
			} else {
				route := "http://" + contentNode + ":8080/content?website=" + host + "&route=" + strings.Replace(path, "/", "%2f", -1)
				withLink := strings.Replace(loaderHTML, "{EDGEHOST}", route, 1)
				withLinkAndHash := strings.Replace(withLink, "{EXPECTEDHASH}", expectedHash[host][path], 1)
				withLinkAndRoute := strings.Replace(withLinkAndHash, "{ROUTE}", strings.Replace(path, "/", "%2f", -1), 1)
				ctx.SetContentType("text/html")
				ctx.SetBody([]byte(withLinkAndRoute))
			}
		} else if noCacheRoutes[host][path] { // Route is explicitly not cached, proxy it
			proxyRequest(ctx, hosts[host]+path)
		} else {
			ctx.SetBody([]byte("404 Not found"))
			ctx.SetStatusCode(fasthttp.StatusNotFound)
		}
	}
}

func chooseContentNode(ipStr string, netState *state.NetworkState) (string, error) {
	var contentNode string
	var nodeErr error

	if viper.GetInt("TEST_LOCAL") == 1 {
		contentNode = "localhost"
	} else if viper.GetInt("ROUND_ROBIN") == 1 {
		contentNode, nodeErr = getNextNode(netState)
	} else {
		contentNode, nodeErr = getClosestNode(ipStr, netState)
	}
	return contentNode, nodeErr
}

func proxyRequest(ctx *fasthttp.RequestCtx, url string) {
	c := &fasthttp.Client{}

	// Transfer the header to a GET request
	statusCode, body, err := c.Get([]byte(ctx.Request.Header.String()), url)
	if err != nil {
		log.Fatalf("Error proxying page: %s", err)
	}
	ctx.SetBody(body)
	ctx.SetStatusCode(statusCode)
}

// getClosestNode wraps the GetClosestNode function from the 'state'
// package to lookup the geographically closest content node to a
// given IP address
func getClosestNode(ipStr string, netState *state.NetworkState) (string, error) {
	ip := net.ParseIP(ipStr)
	if ip == nil {
		log.Printf("Could not parse IP address: %v", ip)
		return "localhost", fmt.Errorf("Could not parse IP address: %v", ip)
	}
	closestNode, err := netState.GetClosestNode(ip)
	if err != nil {
		log.Print(err)
		return "localhost", err
	}
	return closestNode.IP().String(), nil
}

func getNextNode(netState *state.NetworkState) (string, error) {
	nextNode, err := netState.GetNextNode()
	if err != nil {
		return "", err
	}
	return nextNode.IP().String(), nil
}
