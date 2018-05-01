package networking

import (
	"fmt"
	"github.com/valyala/fasthttp"
	"log"
	"net/url"
)

// StartProxy - Start a proxy server
func StartProxy() {
	fmt.Println("Starting...")

	// TODO: This needs to be a thread safe mapping that is loaded from the controld continually.
	hosts := make(map[string]string)
	hosts["google.com"] = "http://172.217.7.228"

	// noCacheRoutes := make(map[string]string)
	// cachedRoutes := make(map[string]string)

	fasthttp.ListenAndServe(":8080", requestBuilder(hosts))
}

func requestBuilder(hosts map[string]string) func(ctx *fasthttp.RequestCtx) {
	// The actual serving function
	return func(ctx *fasthttp.RequestCtx) {
		host := string(ctx.Host()[:])
		if hosts[host] != "" {
			c := &fasthttp.Client{}

			u, err := url.Parse(string(ctx.RequestURI()[:]))
			if err != nil {
				log.Fatal(err)
			}
			path := u.RequestURI()

			// Transfer the header to a GET request
			statusCode, body, err := c.Get([]byte(ctx.Request.Header.String()), hosts[host]+path)
			if err != nil {
				log.Fatalf("Error proxying page: %s", err)
			}
			useResponseBody(ctx, body, statusCode)

		} else {
			ctx.Error("Unsupported Host", fasthttp.StatusBadRequest)
		}
	}
}

func useResponseBody(ctx *fasthttp.RequestCtx, body []byte, statusCode int) {
	ctx.SetBody(body)
	ctx.SetStatusCode(statusCode)
}
