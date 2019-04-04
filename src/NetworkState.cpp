#include <chrono>

#include <folly/dynamic.h>
#include <folly/json.h>

#include "NetworkState.h"

using namespace std::chrono;

NetworkState::NetworkState(P2PConfig& config):
    config_(config)
{
    httpClient_ = std::make_unique<httplib::Client>(
        config.gatewayHost.c_str(),
        config.gatewayPort,
        config.pollInterval /* timeout in seconds */
    );
    if (config_.geoipEnabled) {
        try {
            geo_ = std::make_unique<Geo>(
                config_.geoipDBPath);
        } catch (const std::system_error& e) {
            LOG(ERROR) << "Could not instantiate Geo module\n" << e.what();
        }
    }
}

NetworkState::NetworkState(P2PConfig& config,
    std::unique_ptr<Geo> g): config_(config), geo_(std::move(g)) {
    httpClient_ = std::make_unique<httplib::Client>(
    config.gatewayHost.c_str(),
    config.gatewayPort,
    config.pollInterval /* timeout in seconds */);
}

NetworkState::~NetworkState() {
    fs.shutdown();
}

void NetworkState::parseStateUpdate(std::string body,
    bool ignoreHeartbeat=false) {
    folly::dynamic state = folly::parseJson(body); // full json state
    auto nodeMap = state["response"]["node_data_map"]; // map of content nodes
    int64_t time = duration_cast<seconds>(
        system_clock::now().time_since_epoch()).count(); // current time in ms
    // create new node list with new nodes
    std::vector<std::shared_ptr<EdgeNode>> newList;
    for (auto& pair : nodeMap.items()) {
        // eth address of content node
        std::string nodeAddress = pair.first.getString();
        // lower case eth address of content node
        std::transform(nodeAddress.begin(), nodeAddress.end(),
            nodeAddress.begin(), ::tolower);
        auto& value = pair.second;
        try {
            std::string ip = value["ip_address"]["data"].getString();
            uint16_t port = value["content_port"]["data"].asInt();
            int64_t heartbeat = value["heartbeat"]["data"].asInt();
            bool hasNoContent = value["disk_content"]["data"].empty();
            if (!ignoreHeartbeat && (time - heartbeat) > (2 * 60)) continue;
            if (hasNoContent) continue;
            std::shared_ptr<EdgeNode> node = std::make_shared<EdgeNode>(
                ip, port, nodeAddress, heartbeat);
            if (config_->geo_ip_enabled) 
                node->setLocation(geo_->lookupCoordinates(ip));
            newList.push_back(node);
        } catch (const std::exception& e) {
            LOG(ERROR) << 
                "Caught exception when parsing network state: " << e.what();
        }
    }
    
    if (config_->geo_ip_enabled) {
        // create new KD-Tree with new LockedNodeList
        auto newTreeData = geo_->buildTreeData(newList);
        // swap the old one with the new one
        geo_->setTreeData(newTreeData);
    }
    // swap the old LockedNodeList out with the new one
    { // critical section
        edgeNodes_ = newList;
    }
}

std::vector<std::string> NetworkState::getEdgeNodeHostnames() const {
    std::vector<std::string> v;
    { // critical section for edgeNodes lock
        auto lockedList = edgeNodes_.wlock();
        for (auto& node : *lockedList) {
            std::string fqdn = 
                node->getFQDN(config_->pool_domain, config_->cdn_subdomain);
            v.push_back(fqdn);
        }
    }
    return v;
}

void NetworkState::setEdgeNodes(
    std::vector<std::shared_ptr<EdgeNode>> nodes) {
    { // critical section
        edgeNodes_ = nodes;
    }
}

// return a copy of the list of edge node pointers
std::vector<std::shared_ptr<EdgeNode>> NetworkState::getEdgeNodes() {
    { // critical section
        return edgeNodes_.copy();
    }
}

// performs an N-nearest-neighbor search for up to n edge nodes around
// a geographic location l
std::vector<std::shared_ptr<EdgeNode>> 
    NetworkState::getNearestEdgeNodes(Location l, int n) {
    std::vector<size_t> indices = geo_->getNearestNodes(l, n);
    std::vector<std::shared_ptr<EdgeNode>> nodes;
    for (auto i : indices) {
        nodes.push_back(edgeNodes_.rlock()->at(i));
    }
    return nodes;
}

// looks up the geographic location of an ip address "ip"
// performs an N-nearest-neighbor search for up to n edge nodes around
// it.
std::vector<std::shared_ptr<EdgeNode>> 
    NetworkState::getNearestEdgeNodes(std::string ip, int n) {
    return getNearestEdgeNodes(geo_->lookupCoordinates(ip), n);
}

// Start a separate thread to periodically poll the network
// gateway for state. Calls parseStateUpdate()
void NetworkState::beginPollingGateway() {
    fs.addFunction([&] {
        LOG(INFO) << "Fetching network state from gateway...";
        // Make a GET request to the Gladius network gateway
        // to fetch state
        auto res = httpClient_->Get("/api/p2p/state");
        if (res && res->status == 200) {
            LOG(INFO) << "Received network state from gateway";
            // parse the JSON into state structs/classes
            parseStateUpdate(res->body, config_.ignoreHeartbeat);
        }
    }, std::chrono::seconds(config_.pollInterval), "GatewayPoller");
    fs.setSteady(true);
    fs.start();
    LOG(INFO) << "Started network state polling thread...";
}
