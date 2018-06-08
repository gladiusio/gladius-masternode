package state

import (
	"net"
	"testing"

	"github.com/hongshibao/go-kdtree"
)

func TestKDTree(t *testing.T) {
	var nodes [4]kdtree.Point
	nodes[0] = NewNetworkNode(0.1, 0.1, net.ParseIP("0.1.0.1"))
	nodes[1] = NewNetworkNode(20.0, 20.0, net.ParseIP("20.0.20.0"))
	nodes[2] = NewNetworkNode(30.0, 20.0, net.ParseIP("30.0.20.0"))
	nodes[3] = NewNetworkNode(-40.0, 20.0, net.ParseIP("40.0.20.0"))

	state := &NetworkState{}
	state.tree = kdtree.NewKDTree(nodes[:])

	// Make sure the closest node to (0.0, 0.0) is 0.1.0.1
	closest := state.tree.KNN(NewNetworkNode(0.0, 0.0, net.ParseIP("0.0.0.0")), 1)
	if len(closest) == 0 {
		t.Error("No neighbors were returned!")
	}
	if closest[0].(*NetworkNode).IP().String() != "0.1.0.1" {
		t.Log("KD-Tree nearest neighbor search did not return the correct node!")
		t.Log("Expected: 0.1.0.1")
		t.Logf("Found: %v", closest[0].(*NetworkNode).IP().String())
		t.Fail()
	}

	// Make sure the closest node to (-50.0, -20.0) is 40.0.20.0
	closest = state.tree.KNN(NewNetworkNode(-50.0, -20.0, net.ParseIP("50.0.20.0")), 1)
	if len(closest) == 0 {
		t.Error("No neighbors were returned!")
	}
	if closest[0].(*NetworkNode).IP().String() != "40.0.20.0" {
		t.Log("KD-Tree nearest neighbor search did not return the correct node!")
		t.Log("Expected: 40.0.20.0")
		t.Logf("Found: %v", closest[0].(*NetworkNode).IP().String())
		t.Fail()
	}
}
