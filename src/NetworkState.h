#pragma once

#include <folly/experimental/FunctionScheduler.h>
#include <folly/Synchronized.h>

#include "httplib.h"

#include "Config.h"
#include "EdgeNode.h"
#include "Location.h"
#include "Geo.h"

typedef folly::Synchronized<std::vector
    <std::shared_ptr<EdgeNode>>> LockedNodeList;

class NetworkState {
    private:
        P2PConfig config_;

        // List of edge node data classes
        LockedNodeList edgeNodes_;

        // Used to fetch p2p network state on a repeated basis
        folly::FunctionScheduler fs;

        // Used to make simple requests to the local gladius network gateway
        std::unique_ptr<httplib::Client> httpClient_{nullptr};

        // Used to perform geographic lookups
        std::unique_ptr<Geo> geo_{nullptr};

    public:
        explicit NetworkState(P2PConfig& config);
        explicit NetworkState(P2PConfig& config,
            std::unique_ptr<Geo> g);
        ~NetworkState();

        // return a vector of all edge nodes FQDN's
        std::vector<std::string> getEdgeNodeHostnames() const;

        // Accepts body as a string containing JSON response
        // from the network gateway state request. Parses
        // this response and sets corresponding fields
        // of this NetworkState class. Set ignoreHeartbeat to
        // true if you don't want nodes to be excluded due to
        // old heartbeats.
        void parseStateUpdate(std::string body, bool ignoreHeartbeat);

        // Start a separate thread to periodically poll the network
        // gateway for state. Calls parseStateUpdate()
        void beginPollingGateway();

        void setEdgeNodes(std::vector<std::shared_ptr<EdgeNode>> nodes);
        std::vector<std::shared_ptr<EdgeNode>> getEdgeNodes();
        std::vector<std::shared_ptr<EdgeNode>> 
            getNearestEdgeNodes(Location l, int n);
        std::vector<std::shared_ptr<EdgeNode>> 
            getNearestEdgeNodes(std::string ip, int n);
};
