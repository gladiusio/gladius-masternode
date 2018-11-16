#include "ProxyHandler.h"

#include <iostream>

#include <folly/io/async/EventBaseManager.h>

#include <proxygen/httpserver/RequestHandler.h>

using namespace proxygen;

namespace masternode {
    ProxyHandler::ProxyHandler(folly::CPUThreadPoolExecutor *cpuPool)
        : cpuPool_(cpuPool) {}
    
    void ProxyHandler::onRequest(std::unique_ptr<HTTPMessage> headers) noexcept {
        std::cout << "Received request" << std::endl;
    }
    void ProxyHandler::onBody(std::unique_ptr<folly::IOBuf> body) noexcept {}
    void ProxyHandler::onUpgrade(UpgradeProtocol protocol) noexcept {}
    void ProxyHandler::onEOM() noexcept {}
    void ProxyHandler::requestComplete() noexcept { delete this; }
    void ProxyHandler::onError(ProxygenError err) noexcept { delete this; }
}
