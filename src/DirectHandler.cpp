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
        state_(state) {}

void DirectHandler::onRequest(std::unique_ptr<HTTPMessage> headers) noexcept {
    // Construct network state json response
    const auto& edgeAddrs = state_->getEdgeNodes(); // vector of edge node addresses
    const auto& assetMap = cache_->getAssetHashMap(); // map of urls : hashes

    folly::dynamic jsonRes = folly::dynamic::object;
    jsonRes["edgeNodes"] = folly::dynamic::array;
    for (auto edge : edgeAddrs) {
        jsonRes["edgeNodes"].push_back(edge);
    }
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
