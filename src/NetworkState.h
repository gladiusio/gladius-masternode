#pragma once

#include <folly/experimental/FunctionScheduler.h>
#include <folly/Synchronized.h>

#include "httplib.h"
#include "MasternodeConfig.h"
#include "EdgeNode.h"
#include "Location.h"
#include "Geo.h"

typedef folly::Synchronized<std::vector
    <std::shared_ptr<EdgeNode>>> LockedNodeList;

class NetworkState {
    private:
        std::shared_ptr<MasternodeConfig> config_;

        // List of edge node data classes
        LockedNodeList edgeNodes_;

        // Used to fetch p2p network state on a repeated basis
        folly::FunctionScheduler fs;

        // Used to make simple requests to the local gladius network gateway
        std::unique_ptr<httplib::Client> httpClient_;

        std::unique_ptr<Geo> geo_;

    public:
        explicit NetworkState(std::shared_ptr<MasternodeConfig> config);
        explicit NetworkState(std::shared_ptr<MasternodeConfig> config, std::unique_ptr<Geo> g);
        ~NetworkState();

        std::vector<std::string> getEdgeNodeHostnames();

        // Accepts body as a string containing JSON response
        // from the network gateway state request. Parses
        // this response and sets corresponding fields
        // of this NetworkState class.
        void parseStateUpdate(std::string, bool);
        void beginPollingGateway();

        void setEdgeNodes(std::vector<std::shared_ptr<EdgeNode>> nodes);

        std::vector<std::shared_ptr<EdgeNode>> getNearestEdgeNodes(Location l, int n);
};
