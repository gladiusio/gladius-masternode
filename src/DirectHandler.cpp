#include <proxygen/httpserver/ResponseBuilder.h>

#include "DirectHandler.h"

using namespace proxygen;

DirectHandler::DirectHandler(ContentCache *cache, MasternodeConfig *config,
    NetworkState *state):
        cache_(cache),
        config_(config),
        state_(state) {}

DirectHandler::~DirectHandler() {}

void DirectHandler::onRequest(std::unique_ptr<HTTPMessage> headers) noexcept {
    // Construct network state json response
    auto edgeAddrs = state_->getEdgeNodes(); // vector of edge node addresses
    auto assetMap = cache_->getAssetHashMap(); // map of urls : hashes

    // todo: create json dynamic and send it to requester

    // ResponseBuilder(downstream_)
    //     .status(200, "OK")
    //     .body(/* state json */)
    //     .sendWithEOM();
}

void DirectHandler::onBody(std::unique_ptr<folly::IOBuf> body) noexcept {}
void DirectHandler::onUpgrade(UpgradeProtocol protocol) noexcept {}
void DirectHandler::onEOM() noexcept {}
void DirectHandler::requestComplete() noexcept { delete this; }
void DirectHandler::onError(proxygen::ProxygenError err) noexcept { delete this; }
