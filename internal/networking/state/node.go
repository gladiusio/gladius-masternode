package state

import (
	"fmt"
	"net"

	"github.com/golang/geo/s2"
	kdtree "github.com/hongshibao/go-kdtree"
)

// NetworkNode is a struct representing an edge node in a pool
type NetworkNode struct {
	kdtree.Point // Implements Point interface

	geoLocation  s2.LatLng
	ip           net.IP
	port         int
	ContentFiles []string
}

// NewNetworkNode returns a new NetworkNode struct
func NewNetworkNode(lat, long float64, ip net.IP, port int) *NetworkNode {
	return &NetworkNode{geoLocation: s2.LatLngFromDegrees(lat, long), ip: ip, port: port}
}

func (n *NetworkNode) String() string {
	return fmt.Sprintf("%v [%v, %v]", n.ip, n.geoLocation.Lat.Degrees(), n.geoLocation.Lng.Degrees())
}

func (n *NetworkNode) StringAddress() string {
	return fmt.Sprintf("%s:%d", n.ip, n.port)
}

// IP returns the IP address of the NetworkNode
func (n *NetworkNode) IP() net.IP {
	return n.ip
}

// Dim returns the number of dimensions the KD-Tree splits on
func (n *NetworkNode) Dim() int {
	return 2
}

// GetValue returns the value associated with the provided dimension
func (n *NetworkNode) GetValue(dim int) float64 {
	if dim == 0 {
		return n.geoLocation.Lat.Degrees()
	}
	return n.geoLocation.Lng.Degrees()
}

// Distance returns the calculated distance between two nodes
func (n *NetworkNode) Distance(other kdtree.Point) float64 {
	return n.geoLocation.Distance(other.(*NetworkNode).geoLocation).Radians()
}

// PlaneDistance returns the distance between the point and the specified
// plane
func (n *NetworkNode) PlaneDistance(val float64, dim int) float64 {
	var plane s2.LatLng
	if dim == 0 {
		plane = s2.LatLngFromDegrees(val, 0)
	} else {
		plane = s2.LatLngFromDegrees(0, val)
	}
	return n.geoLocation.Distance(plane).Radians()
}
