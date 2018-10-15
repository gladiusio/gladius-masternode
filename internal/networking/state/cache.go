package state

import (
	"crypto/sha256"
	"fmt"
	"io/ioutil"
	"os"
	"sync"

	"github.com/hongshibao/go-kdtree"

	"github.com/Workiva/go-datastructures/trie/ctrie"
	"github.com/spf13/viper"
	"github.com/tidwall/gjson"
)

/****************************************************************************************/

// Cache is the top-level struct used to maintain mappings of hostnames
// to their routes
type Cache struct {
	// A collection of ProtectedHost structs representing websites under this node's protection
	Hosts *ctrie.Ctrie
}

// Constructs and returns a pointer to a new Cache struct
func newCache() *Cache {
	return &Cache{ctrie.New(nil)}
}

func initCache(c *Cache) {
	// Open cache primer file if given
	primer := viper.GetString("CACHE_PRIMER")
	if primer != "" {
		fmt.Println("Loading cache from primer file...")
		jsonFile, err := os.Open(primer)
		if err != nil {
			fmt.Println(err)
			return
		}
		defer jsonFile.Close()
		jsonBytes, err := ioutil.ReadAll(jsonFile)
		if err != nil {
			fmt.Println(err)
			return
		}
		hostname := gjson.GetBytes(jsonBytes, "hostname").String()
		host := newProtectedHost(hostname)

		routes := gjson.GetBytes(jsonBytes, "routes")
		for _, route := range routes.Array() {
			host.AddRoute(newRoute(route.Get("path").String(), false, route.Get("hash").String()))
		}
		c.AddHost(host)
	}
}

// LookupHost attempts to lookup the ProtectedHost entry in the Cache
// for a given hostname string. Returns the ProtectedHost struct pointer
// if found, nil if not found.
func (c *Cache) LookupHost(hostname string) *ProtectedHost {
	host, exists := c.Hosts.Lookup([]byte(hostname))
	if !exists {
		return nil
	}
	return host.(*ProtectedHost)
}

// AddHost adds a new ProtectedHost pointer to the Cache's hosts
func (c *Cache) AddHost(host *ProtectedHost) {
	c.Hosts.Insert([]byte(host.Hostname), host)
}

/****************************************************************************************/

// ProtectedHost describes a host protected by this masternode as well as its routes
type ProtectedHost struct {
	Hostname   string       // The FQDN of the host to be protected, i.e. "demo.gladius.io"
	Routes     *ctrie.Ctrie // The set of routes known for this host map as http route : *route struct
	FileLookup *ctrie.Ctrie // A lookup table for finding routes based on their asset name
}

// Constructs and returns a pointer to a new ProtectedHost struct
func newProtectedHost(hostname string) *ProtectedHost {
	return &ProtectedHost{
		Hostname:   hostname,
		Routes:     ctrie.New(nil),
		FileLookup: ctrie.New(nil),
	}
}

// AddRoute inserts a pointer to a Route struct into the routes ctrie
// of a ProtectedHost
func (h *ProtectedHost) AddRoute(route *Route) {
	h.Routes.Insert([]byte(route.Route), route)
	h.FileLookup.Insert([]byte(route.Hash), route)
}

// LookupRoute attempts to lookup the Route entry under a ProtectedHost.
// Returns a pointer to the Route struct if it is found, nil otherwise.
func (h *ProtectedHost) LookupRoute(routepath string) *Route {
	route, exists := h.Routes.Lookup([]byte(routepath))
	if !exists {
		return nil
	}
	return route.(*Route)
}

// LookupRouteByFile attempts to lookup the Route entry under a ProtectedHost
// by its file name.
// Returns a pointer to the Route struct if it is found, nil otherwise.
func (h *ProtectedHost) LookupRouteByFile(filename string) *Route {
	route, exists := h.FileLookup.Lookup([]byte(filename))
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

// Route describes a particular route or asset that is known to exist for a
// ProtectedHost
type Route struct {
	Route   string // The route string, i.e. "/products" or "/images/foo.jpg"
	Nocache bool   // Set to true if this route should never be cached
	Hash    string // The hash of the object stored at this route

	Seeders    *kdtree.KDTree // KD-Tree of nodes that have this content
	seedersMux sync.Mutex
}

// Constructs and returns a pointer to a new Route struct
func newRoute(route string, nocache bool, hash string) *Route {
	return &Route{
		Route:   route,
		Nocache: nocache,
		Hash:    hash,
		Seeders: nil,
	}
}

// MakeSeedersTree takes an array of *NetworkNode's and creates
// a KD-tree of them to store in the Route struct
func (r *Route) MakeSeedersTree(seeders []*NetworkNode) {
	nodesAsPoints := make([]kdtree.Point, len(seeders))
	for i := range seeders {
		nodesAsPoints[i] = seeders[i]
	}
	newTree := kdtree.NewKDTree(nodesAsPoints)
	r.seedersMux.Lock()
	r.Seeders = newTree
	r.seedersMux.Unlock()
}
