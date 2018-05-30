package http

import (
	"fmt"
	"time"

	"github.com/tidwall/gjson"
	"github.com/valyala/fasthttp"
)

// GetJSONBytes queries an HTTP endpoint and returns the JSON response
// payload as a byte array
func GetJSONBytes(url string) ([]byte, error) {
	statusCode, body, err := fasthttp.GetTimeout(nil, url, 10*time.Second)
	if err != nil {
		return nil, err
	}
	if statusCode >= 400 {
		return nil, fmt.Errorf("Received unsuccessful HTTP status code \"%d\" from: %s", statusCode, url)
	}
	if !gjson.ValidBytes(body) {
		return nil, fmt.Errorf("Received invalid JSON data from: %s", url)
	}

	return body, nil
}
