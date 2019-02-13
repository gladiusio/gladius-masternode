# Gladius Master Node

## Overview
This application runs as the main connection point in the beta network. All
connections are routed through one master node at this point in time.

The masternode can generally be thought of as a custom proxy server built with Facebook's Proxygen library. It's main functions are to proxy requests from clients through to an origin server and cache the response within the Gladius peer-to-peer network. It then injects a service worker into proxied index page responses which allows additional website assets to be fetched from edge nodes within the Gladius network.


## Building

### Build the first stage of the docker container (proxygen environment)
```shell
docker build --target proxygen-env -t gladiusio/proxygen-env .
```

### Build the second stage of the docker container (builds the masternode binary)
```shell
docker build --target masternode-builder -t gladiusio/masternode-builder .
```

### Build the full container that runs the masternode
```shell
docker build -t gladiusio/masternode .
```

## Developing

Besides the main Dockerfile that is used for building and deploying the masternode, there is an additional develop.Dockerfile that can be used with docker-compose to allow for quicker rebuilds when developing for the masternode locally. Build files will be shared between your host machine and the container.

### Build the development container
```shell
docker build -f develop.Dockerfile -t gladiusio/masternode-develop
```

### Run the development container
```shell
docker-compose -f develop-compose.yml run --name devenv dev bash
```

### Debug tests with development container (once inside it)
```shell
make check
cd tests
gdb ./masternode_tests
```

### Copy library headers to your host machine for IntelliSense purposes (optional)
```shell
docker cp <container id>:/usr/local/include <path on host to put header files>
docker cp <container id>:/usr/include <path on host to put header files>
```
Then configure your IDE to include the path you copied the headers to.

### Build the Proxygen docs (optional)
```shell
docker exec -it <container id> /bin/bash
sudo apt-get install doxygen
cd /proxygen
doxygen Doxyfile

(from another host terminal session)
docker cp <container id>:/proxygen/html <path on host to put docs>
```

## Running the Masternode

As of now, we only support deploying the masternode by using the provided Docker container. The environmental requirements make this the easiest approach.

### Command Line Flags

Flag | Description | Example
---- | ----------- | -------
--ip | The IP address to listen for requests on | --ip=0.0.0.0
--port | The port to listen for HTTP requests on | --port=80
--origin_host | The IP/Hostname of the origin server | --origin_host=192.168.2.12
--origin_port | The port of the origin server to connect to | --origin_port=80
--protected_domain | The domain name we are protecting | --protected_domain=www.example.com
--cache_dir | Path to directory to write cached content to | --cache_dir=/home/bob/content_cache/
--gateway_address | IP/Hostname of Gladius network gateway process | --gateway_address=127.0.0.1
--gateway_port | Port to reach the Gladius network gateway process on | --gateway_port=3001
--sw_path | File path of service worker javascript file to inject | --sw_path=/home/bob/my_service_worker.js
--ssl_port | The port to listen for HTTPS requests on | --ssl_port=443
--cert_path | File path to SSL certificate | --cert_path=/home/bob/cert.pem
--key_path | File path to the private key for the SSL cert | --key_path=/home/bob/key.pem
--upgrade_insecure | Set to true to redirect HTTP requests to the HTTPS port | --upgrade_insecure=true
--logtostderr | Set to 1 to write logs to stderr instead of /tmp files | --logtostderr=1
