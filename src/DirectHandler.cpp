#include <proxygen/httpserver/ResponseBuilder.h>

#include <folly/dynamic.h>
#include <folly/json.h>

#include "DirectHandler.h"

using namespace proxygen;

DirectHandler::DirectHandler(std::shared_ptr<ContentCache> cache, 
    std::shared_ptr<MasternodeConfig> config,
    std::shared_ptr<NetworkState> state):
        cache_(cache),
        config_(config),
        state_(state) {
            CHECK(cache_) << "Cache object was null";
            CHECK(config_) << "Config object was null";
}

void DirectHandler::onRequest(std::unique_ptr<HTTPMessage> headers) noexcept {
    // Construct network state json response

    // top level JSON response object
    folly::dynamic jsonRes = folly::dynamic::object;
    std::vector<std::shared_ptr<EdgeNode>> edgeNodes;
    if (config_->enableP2P && state_) {
        // if geoip is on, use nearest neighbor edge nodes
        if (config_->geo_ip_enabled) {
            edgeNodes = state_->getNearestEdgeNodes(headers->getClientIP(), 5);
        } else { // else send all edge node addresses for now (random in future?)
            edgeNodes = state_->getEdgeNodes();
        }
        // array of edge node http addresses
        jsonRes["edgeNodes"] = folly::dynamic::array;
        for (auto edge : edgeNodes) {
            jsonRes["edgeNodes"].push_back(
                edge->getFQDN(config_->pool_domain, config_->cdn_subdomain));
        }
    }

    const auto& assetMap = cache_->getAssetHashMap(); // map of urls : hashes
    jsonRes["assetHashes"] = folly::dynamic::object;
    for (auto kv : *assetMap.get()) {
        jsonRes["assetHashes"][kv.first] = kv.second;
    }

    ResponseBuilder(downstream_)
        .status(200, "OK")
        .header("Content-Type", "application/json")
        .body(folly::toJson(jsonRes))
        .sendWithEOM();
}

void DirectHandler::onBody(std::unique_ptr<folly::IOBuf> body) noexcept {}
void DirectHandler::onUpgrade(UpgradeProtocol protocol) noexcept {}
void DirectHandler::onEOM() noexcept {}
void DirectHandler::requestComplete() noexcept { delete this; }
void DirectHandler::onError(proxygen::ProxygenError err) 
    noexcept { delete this; }
