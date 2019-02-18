#include <chrono>
#include <algorithm>

#include <folly/dynamic.h>
#include <folly/json.h>

#include "NetworkState.h"

using namespace std::chrono;

NetworkState::NetworkState(std::shared_ptr<MasternodeConfig> config) {
    config_ = config;
    httpClient_ = std::make_unique<httplib::Client>(
        config_->gateway_address.c_str(),
        config->gateway_port,
        config_->gateway_poll_interval /* timeout in seconds */
    );
}

NetworkState::~NetworkState() {
    fs.shutdown();
}

void NetworkState::parseStateUpdate(std::string body, bool ignoreHeartbeat=false) {
    folly::dynamic state = folly::parseJson(body); // full json state
    auto lockedList = edgeNodes_.wlock();
    lockedList->clear();
    auto nodeMap = state["response"]["node_data_map"]; // map of content nodes
    int64_t time = duration_cast<seconds>(
        system_clock::now().time_since_epoch()).count(); // current time in ms
    for (auto& pair : nodeMap.items()) {
        std::string nodeAddress = pair.first.getString(); // eth address of content node
        std::transform(nodeAddress.begin(), nodeAddress.end(),
            nodeAddress.begin(), ::tolower); // lower case eth address of content node
        auto& value = pair.second;
        try {
            std::string ip = value["ip_address"]["data"].getString();
            uint16_t port = value["content_port"]["data"].asInt();
            int64_t heartbeat = value["heartbeat"]["data"].asInt();
            bool hasNoContent = value["disk_content"]["data"].empty();
            if (!ignoreHeartbeat && (time - heartbeat) > (2 * 60)) continue;
            if (hasNoContent) continue;
            std::shared_ptr<EdgeNode> node = std::make_shared<EdgeNode>(
                ip, port, nodeAddress, heartbeat
            );
            lockedList->push_back(node);
        } catch (const std::exception& e) {
            LOG(ERROR) << "Caught exception when parsing network state: " << e.what() << "\n";
        }
    }
}

std::vector<std::string> NetworkState::getEdgeNodeHostnames() {
    std::vector<std::string> v;
    { // critical section for edgeNodes lock
        auto lockedList = edgeNodes_.wlock();
        for (auto& node : *lockedList) {
            std::string fqdn = node->getFQDN(config_->pool_domain, config_->cdn_subdomain);
            v.push_back(fqdn);
        }
    }
    return v;
}

void NetworkState::beginPollingGateway() {
    fs.addFunction([&] {
        LOG(INFO) << "Fetching network state from gateway...\n";
        // Make a GET request to the Gladius network gateway
        // to fetch state
        auto res = httpClient_->Get("/api/p2p/state");
        if (res && res->status == 200) {
            LOG(INFO) << "Received network state from gateway\n";
            // parse the JSON into state structs/classes
            parseStateUpdate(res->body, config_->ignore_heartbeat);
        }
    }, std::chrono::seconds(config_->gateway_poll_interval), "GatewayPoller");
    fs.setSteady(true);
    fs.start();
    LOG(INFO) << "Started network state polling thread...\n";
}
