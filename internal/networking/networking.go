package networking

import (
	"fmt"
	"io/ioutil"
	"log"
	"net/url"
	"strings"

	"github.com/valyala/fasthttp"
)

// StartProxy - Start a proxy server
func StartProxy() {
	fmt.Println("Starting...")

	fmt.Println("Loading route files")
	// read the whole file at once
	loaderHTML, err := ioutil.ReadFile("./html/loader.html")
	if err != nil {
		panic(err)
	}

	// TODO: This needs to be a thread safe mapping that is loaded from the controld continually.
	hosts := make(map[string]string)
	noCacheRoutes := make(map[string]map[string]bool)
	cachedRoutes := make(map[string]map[string]bool)
	expectedHash := make(map[string]map[string]string)

	// Define accepted hosts
	hosts["demo.gladius.io"] = "http://172.217.7.228"
	cachedRoutes["demo.gladius.io"] = make(map[string]bool)
	cachedRoutes["demo.gladius.io"]["/"] = true
	cachedRoutes["demo.gladius.io"]["/anotherroute"] = true

	noCacheRoutes["demo.gladius.io"] = make(map[string]bool)
	noCacheRoutes["demo.gladius.io"]["/api/"] = true

	expectedHash["demo.gladius.io"] = make(map[string]string)
	expectedHash["demo.gladius.io"]["/"] = "734393B3DB3E33F71F4DA4AA4299D994409D5A69C6C7FE99973D7F8F19DEBB53"
	expectedHash["demo.gladius.io"]["/anotherroute"] = "6F9ECF8D1FAD1D2B8FBF2DA3E2571AEC4267A7018DF0DBDE8889D875FBDE8D3F"

	fasthttp.ListenAndServe(":8081", requestBuilder(hosts, cachedRoutes, noCacheRoutes, expectedHash, string(loaderHTML)))
}

func requestBuilder(hosts map[string]string, cachedRoutes, noCacheRoutes map[string]map[string]bool, expectedHash map[string]map[string]string, loaderHTML string) func(ctx *fasthttp.RequestCtx) {
	// The actual serving function
	return func(ctx *fasthttp.RequestCtx) {
		host := string(ctx.Host()[:])
		// Make sure it's a recognized host
		if hosts[host] != "" {
			u, err := url.Parse(string(ctx.RequestURI()[:]))
			if err != nil {
				log.Fatal(err)
			}
			path := u.RequestURI()

			if cachedRoutes[host][path] { // The route is cached, return link to bundle
				ip := ctx.RemoteIP().String()
				closestNode := getClosestNode(ip)

				contentNode := "http://" + closestNode + ":8080/content?website=" + host
				withLink := strings.Replace(loaderHTML, "{EDGEHOST}", contentNode, 1)
				withLinkAndHash := strings.Replace(withLink, "{EXPECTEDHASH}", expectedHash[host][path], 1)
				withLinkAndRoute := strings.Replace(withLinkAndHash, "{ROUTE}", strings.Replace(path, "/", "%2f", -1), 1)

				ctx.SetContentType("text/html")
				ctx.SetBody([]byte(withLinkAndRoute))

			} else if noCacheRoutes[host][path] { // Route is not cached, proxy it
				c := &fasthttp.Client{}

				// Transfer the header to a GET request
				statusCode, body, err := c.Get([]byte(ctx.Request.Header.String()), hosts[host]+path)
				if err != nil {
					log.Fatalf("Error proxying page: %s", err)
				}
				ctx.SetBody(body)
				ctx.SetStatusCode(statusCode)
			} else {
				ctx.SetBody([]byte("404 Not found"))
				ctx.SetStatusCode(fasthttp.StatusNotFound)
			}
		} else {
			ctx.Error("Unsupported Host", fasthttp.StatusBadRequest)
		}
	}
}

func getClosestNode(ip string) string {
	return "localhost"
}
