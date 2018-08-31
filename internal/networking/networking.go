package networking

import (
	"fmt"
	"io/ioutil"
	"log"
	"net"
	"net/url"
	"os"
	"strings"
	"time"

	"github.com/gladiusio/gladius-masternode/internal/http"
	"github.com/gladiusio/gladius-masternode/internal/networking/state"
	"github.com/spf13/viper"
	"github.com/tidwall/gjson"
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
		host := cache.LookupHost(hostname)
		if host == nil {
			ctx.Error("Unsupported Host", fasthttp.StatusBadRequest)
			return
		}

		u, err := url.Parse(string(ctx.RequestURI()[:]))
		if err != nil {
			ctx.Error("Could not parse request URI", fasthttp.StatusBadRequest)
			return
		}
		path := u.RequestURI()
		route := host.LookupRoute(path)
		if route == nil || route.nocache {
			code, content, err := proxyRequest(ctx, host.hostname+path)
			if err != nil {
				log.Printf("Error proxying request: %v\n", err)
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
		contentNode, nodeErr := chooseContentNode(ip, http.JoinStrings(host.hostname, "/", route.hash), networkState)
		if nodeErr != nil {
			proxyRequest(ctx, host.hostname+path)
			return
		}

		contentRoute := http.JoinStrings("http://", contentNode, "/content?website=", host.hostname)
		withLink := strings.Replace(loaderHTML, "{EDGEHOST}", contentRoute, 1)
		withLinkAndHash := strings.Replace(withLink, "{EXPECTEDHASH}", host.LookupRoute(path).hash, 1)
		ctx.SetContentType("text/html")
		ctx.SetBodyString(withLinkAndHash)
	}
}

func initCache(c *Cache) {
	// Open cache primer file if given
	primer := viper.GetString("CACHE_PRIMER")
	if primer != "" {
		jsonFile, err := os.Open(primer)
		if err != nil {
			fmt.Println(err)
			return
		}
		defer jsonFile.Close()
		jsonBytes, err := ioutil.ReadAll(jsonFile)
		if err != nil {
			fmt.Println(err)
			return
		}
		hostname := gjson.GetBytes(jsonBytes, "hostname").String()
		host := newProtectedHost(hostname)

		routes := gjson.GetBytes(jsonBytes, "routes")
		for _, route := range routes.Array() {
			host.AddRoute(newRoute(route.Get("path").String(), false, route.Get("hash").String()))
		}
		c.AddHost(host)
	}
}

// chooseContentNode will return the IP address of the appropriate content node
// to serve content from
func chooseContentNode(ipStr string, fileName string, netState *state.NetworkState) (string, error) {
	var contentNode string
	var nodeErr error

	if viper.GetInt("TEST_LOCAL") == 1 {
		contentNode = "localhost"
	} else if viper.GetInt("ROUND_ROBIN") == 1 {
		contentNode, nodeErr = getNextNode(netState)
	} else {
		contentNode, nodeErr = getClosestServingNode(ipStr, fileName, netState)

	}
	return contentNode, nodeErr
}

func proxyRequest(ctx *fasthttp.RequestCtx, url string) (int, []byte, error) {
	c := &fasthttp.Client{}

	// Transfer the header to a GET request
	fmt.Printf("Proxying page: %s\n", ctx.Request.Header.String())
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
		return "", fmt.Errorf("Could not parse IP address: %v", ip)
	}
	closestNode, err := netState.GetClosestNode(ip)
	if err != nil {
		log.Print(err)
		return "", err
	}
	return closestNode.IP().String(), nil
}

// getClosestServingNode tries to find a nearby content node that is hosting
// the desired file. If it does not find one, an error will be returned
func getClosestServingNode(ipStr string, fileName string, netState *state.NetworkState) (string, error) {
	ip := net.ParseIP(ipStr)
	if ip == nil {
		log.Printf("Could not parse IP address: %v", ip)
		return "", fmt.Errorf("Could not parse IP address: %v", ip)
	}
	closestNodes, err := netState.GetNClosestNodes(ip, 5)
	if err != nil {
		log.Print(err)
		return "", err
	}
	fmt.Printf("Found %d nearest nodes:\n\t%v\n", len(closestNodes), closestNodes)
	// Check these nodes for the desired file
	for _, node := range closestNodes {
		fmt.Printf("Checking node: %v\n", node)
		for _, file := range node.ContentFiles {
			fmt.Printf("Found file: %v\n", file)
			if file == fileName {
				// Found the file in a node
				fmt.Printf("Found file at: %s\n", node.StringAddress())
				return node.StringAddress(), nil
			}
		}
	}
	return "", fmt.Errorf("Could not find the requested content on a nearby node: %v", fileName)
}

func getNextNode(netState *state.NetworkState) (string, error) {
	nextNode, err := netState.GetNextNode()
	if err != nil {
		return "", err
	}
	return nextNode.IP().String(), nil
}
