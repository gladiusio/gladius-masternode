package state

import (
	"fmt"
	"log"
	"net"
	"strconv"
	"strings"
	"sync"
	"time"

	"github.com/gladiusio/gladius-masternode/internal/http"
	"github.com/hongshibao/go-kdtree"
	"github.com/oschwald/geoip2-golang"
	"github.com/spf13/viper"
	"github.com/tidwall/gjson"
)

// NetworkState is a struct containing a K-D tree representation of the edge node pool
// associated with this masternode
type NetworkState struct {
	nodeSet map[*NetworkNode]bool
	tree    *kdtree.KDTree // K-D Tree to store network node structs

	geoIP *geoip2.Reader // Geo IP database reference

	running    bool
	runChannel chan (bool)
	mux        sync.Mutex
}

// NewNetworkState returns a new NetworkState struct
func NewNetworkState() *NetworkState {
	state := &NetworkState{running: true, runChannel: make(chan bool)}

	// Initialize the Geo IP database
	if viper.GetBool("DisableGeoip") == true {
		db, err := InitGeoIP()
		if err != nil {
			log.Fatal(err)
		}
		state.geoIP = db
	}

	// Initialize nodes
	state.RefreshActiveNodes()

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

// RefreshActiveNodes fetches the latest status of nodes in the pool
func (n *NetworkState) RefreshActiveNodes() {
	// Make a request to controld for the currently active nodes
	url := http.BuildControldEndpoint("/api/p2p/state")
	responseBytes, err := http.GetJSONBytes(url)
	if err != nil {
		log.Fatal(err)
	}
	// Parse the 'response' field from the response data
	_nodes := gjson.GetBytes(responseBytes, "response.node_data_map")

	// Update set of active nodes
	n.BuildTree(_nodes)
}

// BuildTree creates a KD Tree of NetworkNodes for use with GeoIP
// node selection
func (n *NetworkState) BuildTree(_nodes gjson.Result) {
	// Create NetworkNode structs from the list of nodes returned from controld
	nodes := make([]kdtree.Point, 0)
	_nodes.ForEach(func(key, value gjson.Result) bool {
		heartbeat := value.Get("heartbeat.data").Int()
		dif := time.Now().Unix() - heartbeat
		if dif > 5 {
			fmt.Printf("Ignoring node with old heartbeat. Difference: %v seconds old\n", dif)
			return true
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
			nodes = append(nodes, newNode)
		}
		return true
	})

	// Create a new KD-Tree with the new set of nodes
	newTree := kdtree.NewKDTree(nodes)
	n.mux.Lock()
	n.tree = newTree
	n.mux.Unlock()
}

func incIP(ip net.IP) {
	for j := len(ip) - 1; j >= 0; j-- {
		ip[j]++
		if ip[j] > 0 {
			break
		}
	}
}

func HostsFromCIDR(cidr string) []net.IP {
	ip, network, err := net.ParseCIDR(cidr)
	if err != nil {
		log.Fatal(err)
	}
	var ips []net.IP
	for ip := ip.Mask(network.Mask); network.Contains(ip); incIP(ip) {
		ipCopy := make(net.IP, len(ip))
		copy(ipCopy, ip)
		ips = append(ips, ipCopy)
	}

	return ips[1 : len(ips)-1]
}

// GeolocateIP looks up the coordinates for a given ip address
func (n *NetworkState) GeolocateIP(ip net.IP) (long float64, lat float64, retErr error) {
	n.mux.Lock()
	defer n.mux.Unlock()
	city, err := n.geoIP.City(ip)
	if err != nil {
		return 0.0, 0.0, err
	}

	return city.Location.Longitude, city.Location.Latitude, nil
}

// GetClosestNode searches the KD-Tree for the node nearest to the IP provided
func (n *NetworkState) GetClosestNode(ip net.IP) (*NetworkNode, error) {
	if n.tree == nil {
		return nil, fmt.Errorf("Could not find closest node, no nodes are available")
	}
	long, lat, err := n.GeolocateIP(ip)
	if err != nil {
		return nil, fmt.Errorf("Error encountered when looking up coordinates for IP: %v\n\n%v", ip, err)
	}
	n.mux.Lock()
	defer n.mux.Unlock()

	// Find the closest neighboring node
	neighbors := n.tree.KNN(NewNetworkNode(long, lat, ip, 0), 1)
	if len(neighbors) == 0 {
		return nil, fmt.Errorf("Error: No neighbors were found in nearest neighbor search")
	}
	return neighbors[0].(*NetworkNode), nil
}

func (net *NetworkState) GetNClosestNodes(ip net.IP, n int) ([]*NetworkNode, error) {
	if net.tree == nil {
		return nil, fmt.Errorf("Could not find closest node, no nodes are available")
	}
	long, lat, err := net.GeolocateIP(ip)
	if err != nil {
		return nil, fmt.Errorf("Error encountered when looking up coordinates for IP: %v\n\n%v", ip, err)
	}
	net.mux.Lock()
	defer net.mux.Unlock()

	// Find the closest neighboring node
	neighbors := net.tree.KNN(NewNetworkNode(long, lat, ip, 0), 1)
	if len(neighbors) == 0 {
		return nil, fmt.Errorf("Error: No neighbors were found in nearest neighbor search")
	}
	nodes := make([]*NetworkNode, len(neighbors))
	for i := range neighbors {
		nodes[i] = neighbors[i].(*NetworkNode)
	}
	return nodes, nil
}

// RunningStateChanged returns a channel that updates when the running state
// is changed
func (n *NetworkState) RunningStateChanged() chan (bool) {
	return n.runChannel
}
