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

## Running the Software

Run `go run cmd/main.go` in the root directory. Make
sure you have 80 forwarded to 8081.

## Building
Run these commands in the root directory:
```sh
make dependencies
make
```

Optionally, you can install and run linting tools:
```sh
go get gopkg.in/alecthomas/gometalinter.v2
gometalinter.v2 --install
make lint
```
