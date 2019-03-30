#pragma once

#include <folly/experimental/FunctionScheduler.h>
#include <folly/Synchronized.h>

#include "httplib.h"
#include "Config.h"

class NetworkState {
    private:
        P2PConfig config_;
        // List of addresses of edge nodes
        folly::Synchronized<std::vector<std::string>> edgeNodes_;
        // Used to fetch p2p network state on a repeated basis
        folly::FunctionScheduler fs;
        // Used to make simple requests to the local gladius network gateway
        std::unique_ptr<httplib::Client> httpClient_{nullptr};

    public:
        explicit NetworkState(P2PConfig& config);
        ~NetworkState();

        std::vector<std::string> getEdgeNodes() const;

        // Accepts body as a string containing JSON response
        // from the network gateway state request. Parses
        // this response and sets corresponding fields
        // of this NetworkState class.
        void parseStateUpdate(std::string, bool);
        std::string createEdgeNodeHostname(std::string, std::string) const;
        void beginPollingGateway();
};


