package state

import (
	"encoding/json"
	"fmt"
	"log"
	"net"
	"strconv"
	"strings"
	"sync"
	"time"

	"github.com/gladiusio/gladius-masternode/internal/http"
	"github.com/oschwald/geoip2-golang"
	"github.com/spf13/viper"
	"github.com/tidwall/gjson"
)

// NetworkState is a struct containing a K-D tree representation of the edge node pool
// associated with this masternode
type NetworkState struct {
	nodeMap map[string]*NetworkNode // Map of all connected edge nodes. maps ethereum address to node pointer

	Cache *Cache // Pointer to the content cache manager

	geoIP *geoip2.Reader // Geo IP database reference

	running    bool
	runChannel chan (bool)
	mux        sync.Mutex
}

// NewNetworkState returns a new NetworkState struct
func NewNetworkState() *NetworkState {
	state := &NetworkState{running: true, runChannel: make(chan bool)}

	// Initialize the Geo IP database
	if viper.GetBool("DISABLE_GEOIP") != true {
		fmt.Println("DisableGeoip not true")
		db, err := InitGeoIP()
		if err != nil {
			log.Fatal(err)
		}
		state.geoIP = db
	}

	// Setup
	cache := newCache()
	initCache(cache)
	state.Cache = cache

	// Initialize nodes and content
	state.RefreshNetworkState()

	return state
}

// SetNetworkRunState updates the desired state of the networking
func (n *NetworkState) SetNetworkRunState(runState bool) {
	n.mux.Lock()
	defer n.mux.Unlock()
	if n.running != runState {
		n.running = runState
		go func() { n.runChannel <- runState }()
	}
}

func (n *NetworkState) RefreshNetworkState() {
	// Make a request to controld for the current p2p network state
	url := http.BuildControldEndpoint("/api/p2p/state")
	responseBytes, err := http.GetJSONBytes(url)
	if err != nil {
		log.Printf("Encountered an error when fetching p2p state from controld: \n%v\n", err)
		return
	}

	n.refreshActiveNodes(responseBytes)
	n.refreshContentTrees(responseBytes)
}

// RefreshActiveNodes fetches the latest status of nodes in the pool
func (n *NetworkState) refreshActiveNodes(stateBytes []byte) {

	// Parse the 'response' field from the response data
	_nodes := gjson.GetBytes(stateBytes, "response.node_data_map")

	// Update set of active nodes
	newNodeMap := make(map[string]*NetworkNode)
	_nodes.ForEach(func(key, value gjson.Result) bool {
		if viper.GetBool("IGNORE_HEARTBEAT") != true {
			fmt.Println("ignore heartbeat not true")
			heartbeat := value.Get("heartbeat.data").Int()
			dif := time.Now().Unix() - heartbeat
			if dif > 5 {
				fmt.Printf("Ignoring node with old heartbeat. %v seconds old\n", dif)
				return true
			}
		}
		ipStr := strings.TrimSpace(value.Get("ip_address.data").String())
		ip := net.ParseIP(ipStr)
		if ip == nil {
			log.Printf("Invalid IP Address found in node: %v", value)
			return true
		}
		portStr := strings.TrimSpace(value.Get("content_port.data").String())
		port, err := strconv.Atoi(portStr)
		if err != nil {
			log.Printf("Invalid port number found in node: %v", value)
			return true
		}
		contentFiles := value.Get("disk_content.data").Array()
		// Lookup approximate coordinates of this IP address
		long, lat, err := n.GeolocateIP(ip)
		if err != nil {
			log.Printf("Error encountered when looking up coordinates for IP: %v\n\n%v", ip, err)
		} else {
			newNode := NewNetworkNode(long, lat, ip, port)
			files := make([]string, len(contentFiles))
			for i := range contentFiles {
				files[i] = contentFiles[i].String()
			}
			newNode.ContentFiles = files
			newNodeMap[key.String()] = newNode
			fmt.Println("added node to nodemap")
		}
		return true
	})
	n.mux.Lock()
	n.nodeMap = newNodeMap
	n.mux.Unlock()
}

func (n *NetworkState) refreshContentTrees(stateBytes []byte) {
	// get list of required files (done above)
	poolData := gjson.GetBytes(stateBytes, "response.pool_data.required_content.data")

	// ask controld content_links endpoint for where to find all these files (have to modify controld code for new endpoint to return structs describing nodes instead of urls)
	url := http.BuildControldEndpoint("/api/p2p/state/content_locations")
	contentRequestJSON := http.ContentRequest{Content: []byte(poolData.String())}
	payload, err := json.Marshal(contentRequestJSON)
	if err != nil {
		log.Printf("Encountered an error when parsing JSON to request locations of content: %v\n", err)
	}

	responseBytes, err := http.PostJSON(url, payload)
	if err != nil {
		log.Printf("Encountered an error when requesting locations of content from the controld: %v\n", err)
	}

	// build kd-tree for each content file
	contentInfo := gjson.GetBytes(responseBytes, "response")
	contentInfo.ForEach(func(key, value gjson.Result) bool {
		substrs := strings.SplitN(key.String(), "/", 2)
		hostname := substrs[0]
		file := substrs[1]
		host := n.Cache.LookupHost(hostname)
		if host == nil {
			log.Printf("Could not find host \"%s\" in cache when building kd-tree for asset: %s\n", hostname, key.String())
			return true
		}
		// see if a route exists for this file
		route := host.LookupRouteByFile(file)
		if route == nil { // Ignore this file. Potentially malicious
			log.Printf("Encountered a file on the p2p network that the masternode did not know about: %s\n", key.String())
			return true
		}

		// lookup nodes by address in nodeMap
		seeders := make([]*NetworkNode, len(value.Array()))
		fmt.Printf("Nodemap: %v\n", n.nodeMap)
		value.ForEach(func(key, value gjson.Result) bool {
			fmt.Printf("Found this address in JSON: %v\n", value.Get("address").String())

			seeders = append(seeders, n.nodeMap[value.Get("address").String()])
			return true
		})
		//route.MakeSeedersTree()

		return true
	})
}

// GeolocateIP looks up the coordinates for a given ip address
func (n *NetworkState) GeolocateIP(ip net.IP) (long float64, lat float64, retErr error) {
	if viper.GetBool("DISABLE_GEOIP") == true {
		return 0.0, 0.0, nil
	}
	n.mux.Lock()
	defer n.mux.Unlock()
	city, err := n.geoIP.City(ip)
	if err != nil {
		return 0.0, 0.0, err
	}

	return city.Location.Longitude, city.Location.Latitude, nil
}

// GetNClosestNodes takes a given IP address and cached asset name and attempts to lookup
// the nearest n nodes to it.
func (net *NetworkState) GetNClosestNodes(ip net.IP, asset string, n int) ([]*NetworkNode, error) {

	// // Check that we know about this asset

	// // Check that this asset is cached on at least one node

	// long, lat, err := net.GeolocateIP(ip)
	// if err != nil {
	// 	return nil, fmt.Errorf("Error encountered when looking up coordinates for IP: %v\n\n%v", ip, err)
	// }

	// // Find the closest neighboring node
	// neighbors := net.tree.KNN(NewNetworkNode(long, lat, ip, 0), n)
	// if len(neighbors) == 0 {
	// 	return nil, fmt.Errorf("Error: No neighbors were found in nearest neighbor search")
	// }
	// nodes := make([]*NetworkNode, len(neighbors))
	// for i := range neighbors {
	// 	nodes[i] = neighbors[i].(*NetworkNode)
	// }
	// return nodes, nil
	return nil, nil
}

// RunningStateChanged returns a channel that updates when the running state
// is changed
func (n *NetworkState) RunningStateChanged() chan (bool) {
	return n.runChannel
}
