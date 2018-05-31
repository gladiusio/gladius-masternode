package http

import (
	"fmt"
	"strings"
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

// JoinStrings ... concatenates an arbitrary amount of provided strings
func JoinStrings(strs ...string) string {
	var builder strings.Builder
	for _, str := range strs {
		builder.WriteString(str)
	}
	return builder.String()
}

// BuildURL constructs a URL string
func BuildURL(protocol string, fqdn string, path string) string {
	var builder strings.Builder
	builder.WriteString(protocol)
	builder.WriteString("://")
	builder.WriteString(fqdn)
	builder.WriteString(path)
	return builder.String()
}
