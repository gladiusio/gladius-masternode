package networking

import (
	"fmt"
	"io/ioutil"
	"log"
	"net"
	"net/url"
	"strings"
	"time"

	"github.com/gladiusio/gladius-masternode/internal/http"
	"github.com/gladiusio/gladius-masternode/internal/networking/state"
	"github.com/spf13/viper"
	"github.com/valyala/fasthttp"
)

// StartProxy - Start a proxy server
func StartProxy() {
	fmt.Println("Starting...")

	fmt.Println("Loading route files")
	// read the whole file at once
	loaderHTML, err := ioutil.ReadFile("./html/iframe_loader.html")
	if err != nil {
		panic(err)
	}

	cache := newCache()
	initCache(cache)

	// Create new network state object to keep track of edge nodes
	netState := state.NewNetworkState()

	go fasthttp.ListenAndServe(":8081", requestBuilder(string(loaderHTML), cache, netState))

	if viper.GetString("MININET_CONFIG") == "" && viper.GetInt("TEST_LOCAL") != 1 {
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

func requestBuilder(loaderHTML string, cache *Cache, networkState *state.NetworkState) func(ctx *fasthttp.RequestCtx) {
	// The actual serving function
	return func(ctx *fasthttp.RequestCtx) {
		hostname := string(ctx.Host()[:])
		// If this host isn't recognized, break out
		fmt.Printf("Looking up host: %s\n", hostname)
		host := cache.LookupHost(hostname)
		if host == nil {
			ctx.Error("Unsupported Host", fasthttp.StatusBadRequest)
			return
		}

		u, err := url.Parse(string(ctx.RequestURI()[:]))
		if err != nil {
			log.Fatal(err)
		}
		path := u.RequestURI()
		route := host.LookupRoute(path)
		if route == nil || route.nocache {
			code, content, err := proxyRequest(ctx, host.hostname+path)
			if err != nil {
				ctx.Error("Error handling request", code)
				return
			}
			if !route.nocache {
				go func() {
					host.CacheRoute(content, path) // Cache this new content
					// TODO (ALEX): Notify the p2p network of new content
				}()
			}
			return
		}
		// Reply from cache
		ip := ctx.RemoteIP().String()
		// Determine which content node will serve the assets for this request
		contentNode, nodeErr := chooseContentNode(ip, networkState)
		if nodeErr != nil {
			proxyRequest(ctx, host.hostname+path)
			return
		}

		contentRoute := http.JoinStrings("http://", contentNode, ":8080/content?website=", host.hostname, "&route=", strings.Replace(path, "/", "%2f", -1))
		withLink := strings.Replace(loaderHTML, "{EDGEHOST}", contentRoute, 1)
		withLinkAndHash := strings.Replace(withLink, "{EXPECTEDHASH}", host.LookupRoute(path).hash, 1)
		withLinkAndRoute := strings.Replace(withLinkAndHash, "{ROUTE}", strings.Replace(path, "/", "%2f", -1), 1)
		ctx.SetContentType("text/html")
		ctx.SetBodyString(withLinkAndRoute)
	}
}

func initCache(c *Cache) {
	// Add the demo.gladius.io host
	host := newProtectedHost("demo.gladius.io")
	// Add routes to the host
	host.AddRoute(newRoute("/", false, "8476da67667d0c127963bf46c3b637935961014ebe155812d6fc7d64a4a37c41"))
	host.AddRoute(newRoute("/anotherroute", false, "6F9ECF8D1FAD1D2B8FBF2DA3E2571AEC4267A7018DF0DBDE8889D875FBDE8D3F"))
	host.AddRoute(newRoute("/api/", true, ""))
	c.AddHost(host)
}

// chooseContentNode will return the IP address of the appropriate content node
// to serve content from
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

func proxyRequest(ctx *fasthttp.RequestCtx, url string) (int, []byte, error) {
	c := &fasthttp.Client{}

	// Transfer the header to a GET request
	statusCode, body, err := c.Get([]byte(ctx.Request.Header.String()), url)
	if err != nil {
		return statusCode, nil, fmt.Errorf("Error proxying page:\n%v", err)
	}
	ctx.SetBody(body)
	ctx.SetStatusCode(statusCode)
	return statusCode, body, nil
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
