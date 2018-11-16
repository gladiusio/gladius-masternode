# Gladius Master Node

The Gladius Master Node software

## Overview
This application runs as a the main connection point in the beta network. All
connections are routed through one master node, and then the client is connected
to the geographically nearest edge node.

## Running the Software



# Building

## Build the base image
```shell
docker build -t gladiusio/masternode-base .
```

## Copy library headers to host for IntelliSense purposes
```shell
docker cp <container id>:/usr/local/include <path to put header files>
```
Then configure your IDE to include the path you copied the headers to.

## Run a container with local source files mounted
```shell
docker run -v $PWD:/src -it gladiusio/masternode-base
```

## Build Masternode inside the container
```shell
cd /src/src
g++ -std=c++14 -o masternode masternode.cpp ProxyHandler.cpp -lproxygenhttpserver -lfolly -lglog -pthread
```
