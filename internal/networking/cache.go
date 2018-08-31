package networking

import (
	"crypto/sha256"
	"strings"

	"github.com/Workiva/go-datastructures/trie/ctrie"
)

/****************************************************************************************/

// Cache is the top-level struct used to maintain mappings of hostnames
// to their routes
type Cache struct {
	hosts *ctrie.Ctrie
}

// Constructs and returns a pointer to a new Cache struct
func newCache() *Cache {
	return &Cache{ctrie.New(nil)}
}

// LookupHost attempts to lookup the ProtectedHost entry in the Cache
// for a given hostname string. Returns the ProtectedHost struct pointer
// if found, nil if not found.
func (c *Cache) LookupHost(hostname string) *ProtectedHost {
	host, exists := c.hosts.Lookup([]byte(hostname))
	if !exists {
		return nil
	}
	return host.(*ProtectedHost)
}

// AddHost adds a new ProtectedHost pointer to the Cache's hosts
func (c *Cache) AddHost(host *ProtectedHost) {
	c.hosts.Insert([]byte(host.hostname), host)
}

/****************************************************************************************/

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

// AddRoute inserts a pointer to a Route struct into the routes ctrie
// of a ProtectedHost
func (h *ProtectedHost) AddRoute(route *Route) {
	h.routes.Insert([]byte(route.route), route)
}

// LookupRoute attempts to lookup the Route entry under a ProtectedHost.
// Returns a pointer to the Route struct if it is found, nil otherwise.
func (h *ProtectedHost) LookupRoute(routepath string) *Route {
	route, exists := h.routes.Lookup([]byte(routepath))
	if !exists {
		return nil
	}
	return route.(*Route)
}

// CacheRoute adds a new route to the collection of cached routes
// along with the sha256 hash of the content
func (h *ProtectedHost) CacheRoute(content []byte, path string) {
	// Get the hash of the content to cache
	hash := sha256.Sum256(content)
	// Add the route to the ProtectedHost's routes
	h.AddRoute(newRoute(path, false, string(hash[:])))
}

/****************************************************************************************/

// Route describes a particular route that is known to exist for a
// ProtectedHost
type Route struct {
	route   string // The route string, i.e. "/products"
	nocache bool   // Set to true if this route should never be cached
	hash    string // The hash of the object stored at this route
}

// Constructs and returns a pointer to a new Route struct
func newRoute(route string, nocache bool, hash string) *Route {
	route = strings.Replace(route, "index.html", "", 1)
	route = strings.Replace(route, ".html", "", 1)
	return &Route{route, nocache, hash}
}
