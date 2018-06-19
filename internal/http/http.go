package http

import (
	"fmt"
	"strings"
	"time"

	"github.com/spf13/viper"
	"github.com/tidwall/gjson"
	"github.com/valyala/fasthttp"
)

// GetJSONBytes queries an HTTP endpoint and returns the JSON response
// payload as a byte array
func GetJSONBytes(url string) ([]byte, error) {
	bytes, err := GetBytes(url)
	if err != nil {
		return nil, err
	}
	if !gjson.ValidBytes(bytes) {
		return nil, fmt.Errorf("Received invalid JSON data from: %s", url)
	}

	return bytes, nil
}

// GetBytes queries an HTTP endpoint and returns the raw []byte response
func GetBytes(url string) ([]byte, error) {
	statusCode, body, err := fasthttp.GetTimeout(nil, url, 10*time.Second)
	if err != nil {
		return nil, err
	}
	if statusCode != 200 {
		return nil, fmt.Errorf("Received unsuccessful HTTP status code \"%d\" from: %s", statusCode, url)
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
func BuildURL(protocol string, host string, port string, route string) string {
	var builder strings.Builder
	builder.WriteString(protocol)
	builder.WriteString("://")
	builder.WriteString(host)
	builder.WriteString(":")
	builder.WriteString(port)
	builder.WriteString(route)
	return builder.String()
}

// BuildControldEndpoint builds a URL for a controld endpoint
func BuildControldEndpoint(route string) string {
	protocol := viper.GetString("ControldProtocol")
	host := viper.GetString("ControldHostname")
	port := viper.GetString("ControldPort")
	return BuildURL(protocol, host, port, route)
}
