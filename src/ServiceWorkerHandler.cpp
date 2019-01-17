#include <proxygen/httpserver/ResponseBuilder.h>

#include "ServiceWorkerHandler.h"

using namespace proxygen;

ServiceWorkerHandler::ServiceWorkerHandler(
    MasternodeConfig *config,
    ServiceWorker *sw):
    config_(config),
    sw_(sw) {}

ServiceWorkerHandler::~ServiceWorkerHandler() {}

void ServiceWorkerHandler::onRequest(std::unique_ptr<proxygen::HTTPMessage> headers) noexcept {
    // reply with service worker content
    ResponseBuilder(downstream_)
        .status(200, "OK")
        .header("Content-Type", "application/javascript")
        .body(sw_->getPayload())
        .sendWithEOM();
}

void ServiceWorkerHandler::onBody(std::unique_ptr<folly::IOBuf> body) noexcept {}
void ServiceWorkerHandler::onUpgrade(proxygen::UpgradeProtocol protocol) noexcept {}
void ServiceWorkerHandler::onEOM() noexcept {}
void ServiceWorkerHandler::requestComplete() noexcept {}
void ServiceWorkerHandler::onError(proxygen::ProxygenError err) noexcept {}
