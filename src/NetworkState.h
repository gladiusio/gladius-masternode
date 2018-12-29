#pragma once

#include <chrono>

#include <folly/experimental/FunctionScheduler.h>
#include <folly/dynamic.h>
#include <folly/json.h>

#include "httplib.h"
#include "MasternodeConfig.h"

class NetworkState {
    private:
        std::shared_ptr<MasternodeConfig> config_;
        // List of addresses of edge nodes
        std::vector<std::string> edgeNodes_;
        // Used to fetch p2p network state on a repeated basis
        folly::FunctionScheduler fs;
        // Used to make simple requests to the local gladius network gateway
        std::unique_ptr<httplib::Client> httpClient_;

    public:
        NetworkState(std::shared_ptr<MasternodeConfig> config) {
            config_ = config;
            httpClient_ = std::make_unique<httplib::Client>(
                config_->gateway_address.c_str(),
                config->gateway_port,
                5 /* timeout in seconds */
            );
        }

        ~NetworkState() {
            fs.shutdown();
        }

        std::vector<std::string> getEdgeNodes() {
            return edgeNodes_;
        }

        // Accepts body as a string containing JSON response
        // from the network gateway state request. Parses
        // this response and sets corresponding fields
        // of this NetworkState class.
        void parseStateUpdate(std::string body) {
            folly::dynamic parsed = folly::parseJson(body);
            auto nodeMap = parsed["response"]["node_data_map"];
            for (auto& value : nodeMap.values()) {
                std::string ip = value["ip_address"]["data"].asString();
                std::string port = value["content_port"]["data"].asString();
                edgeNodes_.push_back(std::string(ip + ":" + port)); // todo: make this atomic/thread-safe
            }
        }

        void beginPollingGateway() {
            fs.addFunction([&] {
                // Make a GET request to the Gladius network gateway
                // to fetch state
                auto res = httpClient_->Get("/api/p2p/state");
                if (res && res->status == 200) {
                    LOG(INFO) << "Received network state from gateway\n";
                    // parse the JSON into state structs/classes
                    parseStateUpdate(res->body);
                }
            }, std::chrono::seconds(5), "GatewayPoller");
            fs.setSteady(true);
            fs.start();
        }
};


