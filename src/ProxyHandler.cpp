#include "ProxyHandler.h"

#include <iostream>

#include <folly/io/async/EventBaseManager.h>

#include <proxygen/httpserver/RequestHandler.h>
#include <proxygen/httpserver/ResponseBuilder.h>
#include <proxygen/lib/http/session/HTTPUpstreamSession.h>

using namespace proxygen;

namespace masternode {
    ProxyHandler::ProxyHandler(folly::CPUThreadPoolExecutor *cpuPool, folly::HHWheelTimer* timer):
        cpuPool_(cpuPool),
        connector_{this, timer},
        originHandler_(*this){}

    ProxyHandler::~ProxyHandler() {}
    
    // RequestHandler methods
    void ProxyHandler::onRequest(std::unique_ptr<HTTPMessage> headers) noexcept {
        std::cout << "Received request" << std::endl;
    }
    void ProxyHandler::onBody(std::unique_ptr<folly::IOBuf> body) noexcept {}
    void ProxyHandler::onUpgrade(UpgradeProtocol protocol) noexcept {}
    void ProxyHandler::onEOM() noexcept {}
    void ProxyHandler::requestComplete() noexcept { delete this; }
    void ProxyHandler::onError(ProxygenError err) noexcept { delete this; }

    // HTTPConnector::Callback methods
    void ProxyHandler::connectSuccess(proxygen::HTTPUpstreamSession* session) noexcept {}
    void ProxyHandler::connectError(const folly::AsyncSocketException& ex) noexcept {}

    // HTTPTransactionHandler delegated methods
    void ProxyHandler::originSetTransaction(proxygen::HTTPTransaction* txn) noexcept {
        originTxn_ = txn;
    }

    void ProxyHandler::originDetachTransaction() noexcept {
        originTxn_ = nullptr;
        // TODO: add code to check for conditions to delete this handler here
    }

    void ProxyHandler::originOnHeadersComplete(std::unique_ptr<proxygen::HTTPMessage> msg) noexcept {
        downstream_->sendHeaders(*(msg.get()));
    }

    void ProxyHandler::originOnBody(std::unique_ptr<folly::IOBuf> chain) noexcept {
        downstream_->sendBody(std::move(chain));
    }

    void ProxyHandler::originOnChunkHeader(size_t length) noexcept {
        downstream_->sendChunkHeader(length);
    }

    void ProxyHandler::originOnChunkComplete() noexcept {
        downstream_->sendChunkTerminator();
    }

    void ProxyHandler::originOnTrailers(std::unique_ptr<proxygen::HTTPHeaders> trailers) noexcept {

    }

    void ProxyHandler::originOnEOM() noexcept {
        downstream_->sendEOM();
    }

    void ProxyHandler::originOnUpgrade(proxygen::UpgradeProtocol protocol) noexcept {

    }

    void ProxyHandler::originOnError(const proxygen::HTTPException& error) noexcept {

    }

    void ProxyHandler::originOnEgressPaused() noexcept {
        originTxn_->pauseIngress();
    }

    void ProxyHandler::originOnEgressResumed() noexcept {
        originTxn_->resumeIngress();
    }

    void ProxyHandler::originOnPushedTransaction(proxygen::HTTPTransaction *txn) noexcept {

    }
}
