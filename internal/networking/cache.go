package networking

import (
	"github.com/Workiva/go-datastructures/trie/ctrie"
)

// ProtectedHost describes a host protected by this masternode as well as its routes
type ProtectedHost struct {
	hostname string       // The FQDN of the host to be protected, i.e. "demo.gladius.io"
	routes   *ctrie.Ctrie // The routes known for this host
}

// Constructs and returns a pointer to a new ProtectedHost struct
func newProtectedHost(hostname string) *ProtectedHost {
	return &ProtectedHost{
		hostname: hostname,
		routes:   ctrie.New(nil),
	}
}

// Route describes a particular route that is known to exist for a
// ProtectedHost
type Route struct {
	route   string // The route string, i.e. "/products"
	nocache bool   // Set to true if this route should never be cached
	hash    string // The hash of the object stored at this route
}

// Constructs and returns a pointer to a new Route struct
func newRoute(route string, nocache bool, hash string) *Route {
	return &Route{route, nocache, hash}
}

// Cache is the top-level struct used to maintain mappings of hostnames
// to their routes
type Cache struct {
	hosts *ctrie.Ctrie
}

// Constructs and returns a pointer to a new Cache struct
func newCache() *Cache {
	return &Cache{ctrie.New(nil)}
}
