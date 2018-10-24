package networking

import (
	"fmt"
	"log"
	"net"
	"net/url"
	"time"

	"github.com/gladiusio/gladius-masternode/internal/networking/state"
	"github.com/valyala/fasthttp"
)

// StartProxy - Start a proxy server
func StartProxy() {
	fmt.Println("Starting...")

	// Create new network state object to keep track of edge nodes
	netState := state.NewNetworkState()

	go fasthttp.ListenAndServe(":8081", requestBuilder(netState))

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

func requestBuilder(networkState *state.NetworkState) func(ctx *fasthttp.RequestCtx) {
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

		if route == nil { // If we haven't seen this route yet
			code, content, err := proxyRequest(ctx, host.Hostname+path) // Proxy the request
			if err != nil {
				log.Printf("Error proxying request: %v\n", err)
				ctx.Error("Error handling request", code)
				return
			}
			go func() {
				host.CacheRoute(content, path) // Cache this new content
				// TODO (ALEX): Notify the p2p network of new content
			}()
			return
		} else if route.Nocache { // If we know that this route is explicitly not to be cached
			code, _, err := proxyRequest(ctx, host.Hostname+path) // Proxy it
			if err != nil {
				log.Printf("Error proxying request: %v\n", err)
				ctx.Error("Error handling request", code)
			}
			return
		}

		// Reply from cache
		//ip := ctx.RemoteIP()
		// Determine which content nodes will serve the assets for this request
		// (later to be injected along with the service worker code to index pages)
		// contentNodes, nodeErr := chooseContentNodes(ip, networkState)
		// if nodeErr != nil {
		// 	proxyRequest(ctx, host.Hostname+path)
		// 	return
		// }
	}
}

func chooseContentNodes(ip net.IP, netState *state.NetworkState) ([]string, error) {
	nearestNeighbors, err := netState.GetNClosestNodes(ip, 5)
	if err != nil {
		return nil, err
	}
	nodeAddresses := make([]string, len(nearestNeighbors))
	for i, node := range nearestNeighbors {
		nodeAddresses[i] = node.StringAddress()
	}
	return nodeAddresses, nil
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
