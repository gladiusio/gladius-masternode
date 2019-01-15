#include "ServiceWorkerHandler.h"

ServiceWorkerHandler::ServiceWorkerHandler(MasternodeConfig *config):
    config_(config)
{
    // load the service worker file from the filesystem
    
}

ServiceWorkerHandler::~ServiceWorkerHandler() {}

void ServiceWorkerHandler::onRequest(std::unique_ptr<proxygen::HTTPMessage> headers) noexcept {}
void ServiceWorkerHandler::onBody(std::unique_ptr<folly::IOBuf> body) noexcept {}
void ServiceWorkerHandler::onUpgrade(proxygen::UpgradeProtocol protocol) noexcept {}
void ServiceWorkerHandler::onEOM() noexcept {}
void ServiceWorkerHandler::requestComplete() noexcept {}
void ServiceWorkerHandler::onError(proxygen::ProxygenError err) noexcept {}
