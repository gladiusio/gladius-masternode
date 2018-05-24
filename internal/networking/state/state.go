package state

import (
	"sync"

	"github.com/hongshibao/go-kdtree"
)

// NetworkState is a struct containing a K-D tree representation of the edge node pool
// associated with this masternode
type NetworkState struct {
	tree *kdtree.KDTree // K-D Tree to store network node structs

	running    bool
	runChannel chan (bool)
	mux        sync.Mutex
}

// NewNetworkState returns a new NetworkState struct
func NewNetworkState() *NetworkState {
	return &NetworkState{running: true, runChannel: make(chan bool)}
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

// RunningStateChanged returns a channel that updates when the running state
// is changed
func (n *NetworkState) RunningStateChanged() chan (bool) {
	return n.runChannel
}

// NetworkNode is a struct representing an edge node in a pool
type NetworkNode struct {
	kdtree.Point // Implements Point interface

	longitude float64
	latitude  float64
	ip        string
}

// NewNetworkNode returns a new NetworkNode struct
func NewNetworkNode(long, lat float64) *NetworkNode {
	return &NetworkNode{longitude: long, latitude: lat, ip: "0.0.0.0"}
}

func (n *NetworkNode) Dim() int {
	return 2
}

func (n *NetworkNode) GetValue(dim int) float64 {
	if dim == 0 {
		return n.longitude
	}
	return n.latitude
}

func (n *NetworkNode) Distance(other kdtree.Point) float64 {
	var ret float64
	for i := 0; i < n.Dim(); i++ {
		tmp := n.GetValue(i) - other.GetValue(i)
		ret += tmp * tmp
	}
	return ret
}

func (n *NetworkNode) PlaneDistance(val float64, dim int) float64 {
	tmp := n.GetValue(dim) - val
	return tmp * tmp
}
