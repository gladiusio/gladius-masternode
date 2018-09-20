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

	// Create new network state object to keep track of edge nodes
	netState := state.NewNetworkState()

	go fasthttp.ListenAndServe(":8081", requestBuilder(string(loaderHTML), netState))

	// Update network state every 30 seconds
	ticker := time.NewTicker(time.Second * 30)
	defer ticker.Stop()
	for {
		select {
		case <-ticker.C:
			netState.RefreshNetworkState()
		}
	}
}

func requestBuilder(loaderHTML string, networkState *state.NetworkState) func(ctx *fasthttp.RequestCtx) {
	// The actual serving function
	return func(ctx *fasthttp.RequestCtx) {
		hostname := string(ctx.Host()[:])
		// If this host isn't recognized, break out
		host := networkState.Cache.LookupHost(hostname)
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
		if route == nil || route.Nocache {
			code, content, err := proxyRequest(ctx, host.Hostname+path)
			if err != nil {
				log.Printf("Error proxying request: %v\n", err)
				ctx.Error("Error handling request", code)
				return
			}
			if !route.Nocache {
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
		contentNode, nodeErr := chooseContentNode(ip, http.JoinStrings(host.Hostname, "/", route.Hash), networkState)
		if nodeErr != nil {
			proxyRequest(ctx, host.Hostname+path)
			return
		}

		// Hydrate the bootstrap HTML document we serve
		contentRoute := http.JoinStrings("http://", contentNode, "/content?website=", host.Hostname)
		withLink := strings.Replace(loaderHTML, "{EDGEHOST}", contentRoute, 1)
		withLinkAndHash := strings.Replace(withLink, "{EXPECTEDHASH}", host.LookupRoute(path).Hash, 1)
		ctx.SetContentType("text/html")
		ctx.SetBodyString(withLinkAndHash)
	}
}

// chooseContentNode will return the IP address of the appropriate content node
// to serve content from
func chooseContentNode(ipStr string, fileName string, netState *state.NetworkState) (string, error) {
	contentNode, nodeErr := getClosestServingNode(ipStr, fileName, netState)

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

// getClosestServingNode tries to find a nearby content node that is hosting
// the desired file. If it does not find one, an error will be returned
func getClosestServingNode(ipStr string, fileName string, netState *state.NetworkState) (string, error) {
	ip := net.ParseIP(ipStr)
	if ip == nil {
		log.Printf("Could not parse IP address: %v", ip)
		return "", fmt.Errorf("Could not parse IP address: %v", ip)
	}
	closestNodes, err := netState.GetNClosestNodes(ip, fileName, 5)
	if err != nil {
		log.Print(err)
		return "", err
	}

	// Check these nodes for the desired file
	for _, node := range closestNodes {
		for _, file := range node.ContentFiles {
			if file == fileName {
				// Found the file in a node
				return node.StringAddress(), nil
			}
		}
	}
	return "", fmt.Errorf("Could not find the requested content on a nearby node: %v", fileName)
}
