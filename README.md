# Gladius Master Node

The Gladius Master Node software

## Overview
This application runs as a the main connection point in the beta network. All
connections are routed through one master node, and then the client is connected
to the geographically nearest edge node.

The master node serves a skeleton site that contains links to the nearest edge
node, the master node also serves Javascript to verify that this edge content
matches the expected content by checking the SHA256 hash against what it should
be.

## RPC Endpoints
These methods are called by the blockchain daemon. This system allows for rapid development on each component by separating them and using standard interfaces. 

Start the master node accepting connections:

`curl -H 'Content-type: application/json' -d '{"jsonrpc": "2.0", "method": "start", "id": 1}' http://localhost:5000/rpc`

Load a list of online nodes into the masternode:

`curl -H 'Content-type: application/json' -d '{"jsonrpc": "2.0", "method": "loadNodes", "params": ["1.1.1.1"], "id": 1}' http://localhost:5000/rpc`
