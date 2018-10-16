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

		if route == nil { // If we haven't seen this route yet
			code, content, err := proxyRequest(ctx, host.Hostname+path) // Proxy the request
			if err != nil {
				log.Printf("Error proxying request: %v\n", err)
				ctx.Error("Error handling request", code)
				return
			}
			go func() {
				host.CacheRoute(content, path) // Cache this new content
				// TODO: create HTML template for this content
				// TODO (ALEX): Notify the p2p network of new content
			}()
			return
		} else if route.Nocache { // If we know that this route is explicitly not to be cached
			code, content, err := proxyRequest(ctx, host.Hostname+path) // Proxy it
			if err != nil {
				log.Printf("Error proxying request: %v\n", err)
				ctx.Error("Error handling request", code)
			}
			return
		}

		// Reply from cache
		ip := ctx.RemoteIP()
		// Determine which content nodes will serve the assets for this request
		contentNodes, nodeErr := chooseContentNodes(ip, route, networkState)
		if nodeErr != nil {
			proxyRequest(ctx, host.Hostname+path)
			return
		}

		// Hydrate the bootstrap HTML document we serve
		contentRoute := http.JoinStrings("http://", contentNode, "/content?website=", host.Hostname)
		withLink := strings.Replace(loaderHTML, "{EDGEHOST}", contentRoute, 1)
		withLinkAndHash := strings.Replace(withLink, "{EXPECTEDHASH}", route.Hash, 1)
		ctx.SetContentType("text/html")
		ctx.SetBodyString(withLinkAndHash)
	}
}

func chooseContentNodes(ip net.IP, route *state.Route, netState *state.NetworkState) ([]string, error) {
	long, lat := 0.0, 0.0
	if viper.GetBool("DISABLE_GEOIP") != true {
		long, lat, err := netState.GeolocateIP(ip)
		if err != nil {
			return nil, fmt.Errorf("Error encountered when looking up coordinates for IP: %v\n\n%v", ip, err)
		}
	}
	nearestNeighbors, err := route.GetNearestNSeeders(5, state.NewNetworkNode(long, lat, ip, 0))
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
