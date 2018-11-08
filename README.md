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

## Run a container with local source files mounted
```shell
docker run -v $PWD:/src -it gladiusio/masternode-base
```
