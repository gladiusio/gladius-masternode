# Gladius Master Node

## Overview
This application runs as the main connection point in the beta network. All
connections are routed through one master node at this point in time.

The masternode can generally be thought of as a custom proxy server built with Facebook's Proxygen library. Its main functions are to proxy requests from clients through to an origin server and cache the response within the Gladius peer-to-peer network. It then injects a service worker into proxied index page responses which allows additional website assets to be fetched from edge nodes within the Gladius network.


## Building


### Build the full container that runs the masternode
```shell
docker build -t gladiusio/masternode .
```

### (Optional) Build the first stage of the docker container (proxygen environment)
```shell
docker build --target proxygen-env -t gladiusio/proxygen-env .
```

### (Optional) Build the second stage of the docker container (builds the masternode binary)
```shell
docker build --target masternode-builder -t gladiusio/masternode-builder .
```


## Developing

Besides the main Dockerfile that is used for building and deploying the masternode, there is an additional develop.Dockerfile that can be used with docker-compose to allow for quicker rebuilds when developing for the masternode locally. Build files will be shared between your host machine and the container.

### Build the development container
```shell
docker build -f develop.Dockerfile -t gladiusio/masternode-develop .
```

### Run the development container
```shell
docker-compose -f develop-compose.yml run --name devenv dev bash
```

### Stop the development container
```shell
docker-compose -f develop-compose.yml down
```

### Build the Masternode inside the dev container
```shell
aclocal && autoconf && automake --add-missing
./configure
make -j8
```

### Debug tests with development container (once inside it)
```shell
make -j8 check
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

To run the masternode container, first make a copy of the `env.list.sample` file from the root of this repo.
```shell
cp env.list.sample env.list
```

Make any edits to the configuration values in this new `env.list` file.

Then you can bring up the container like so:
```shell
docker run -it -p 80:80 -p 443:443 --env-file env.list gladiusio/masternode
```

Be aware that the masternode requires an instance of both the [gladius-network-gateway](https://github.com/gladiusio/gladius-network-gateway) and the [gladius-edged](https://github.com/gladiusio/gladius-edged) (configured as a seed node) processes to be running locally alongside it. This is so it can interact with and distribute content to the Gladius network. You may wish to use Docker volumes to share the content directory between the seed node process and the masternode container. Volumes may also be helpful for providing the service worker javascript file and the SSL certificate/key to the masternode container.

```shell
docker run -it -p 80:80 -p 443:443 -v /home/.gladius/:/gladius --env-file env.list gladiusio/masternode
```
where `/home/.gladius/` on the host machine contains your SSL certificate, certificate key file, content directory for the seed node, and the service worker you wish to inject. Be sure to configure the appropriate paths in the `env.list` file to reflect any usage of Docker volumes.

### Command Line Flags

Flag | Description | Example
---- | ----------- | -------
--ip | The IP address/hostname to listen for requests on | --ip=0.0.0.0
--port | The port to listen for HTTP requests on | --port=80
--origin_host | The IP/Hostname of the origin server to proxy for | --origin_host=192.168.2.12
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
--pool_domain | Domain to use for pool hosts | --pool_domain=examplepool.com
--cdn_subdomain | Subdomain of the pool domain to use for content node hostnames | --cdn_subdomain=cdn1
--ignore_heartbeat | Set to true to disable heartbeat checking for edge nodes | --ignore_heartbeat=true
--logtostderr | Set to 1 to write logs to stderr instead of /tmp files | --logtostderr=1
