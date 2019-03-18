#include "RejectHandler.h"

#include <proxygen/httpserver/ResponseBuilder.h>

using namespace proxygen;

void RejectHandler::onRequest(
    std::unique_ptr<HTTPMessage> headers) noexcept {
    ResponseBuilder(downstream_)
        .status(statusCode_, msg_)
        .sendWithEOM();
}

void RejectHandler::onBody(std::unique_ptr<folly::IOBuf> body) noexcept {}
void RejectHandler::onUpgrade(UpgradeProtocol protocol) noexcept {}
void RejectHandler::onEOM() noexcept {}
void RejectHandler::requestComplete() noexcept { delete this; }
void RejectHandler::onError(proxygen::ProxygenError err) noexcept { 
    LOG(INFO) << "RejectHandler encountered an error: " 
        << getErrorString(err);
    delete this; 
}