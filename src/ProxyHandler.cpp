#include "ProxyHandler.h"

#include <iostream>

#include <folly/io/async/EventBaseManager.h>

#include <proxygen/httpserver/RequestHandler.h>
#include <proxygen/httpserver/ResponseBuilder.h>
#include <proxygen/lib/http/session/HTTPUpstreamSession.h>
#include <proxygen/lib/utils/URL.h>

using namespace proxygen;

namespace masternode {
    ProxyHandler::ProxyHandler(folly::CPUThreadPoolExecutor *cpuPool, folly::HHWheelTimer* timer):
        cpuPool_(cpuPool),
        connector_{this, timer},
        originHandler_(*this){}

    ProxyHandler::~ProxyHandler() {}
    
    // RequestHandler methods
    void ProxyHandler::onRequest(std::unique_ptr<HTTPMessage> headers) noexcept {
        request_ = std::move(headers);
        proxygen::URL url(request_->getURL());

        folly::SocketAddress addr;
        try {
            addr.setFromHostPort("159.203.172.79", 80);
        } catch (...) {
            ResponseBuilder(downstream_)
                .status(503, "Bad Gateway")
                .sendWithEOM();
            return;
        }

        // Stop listening for data from the client while we contact the origin
        downstream_->pauseIngress();
        auto evb = folly::EventBaseManager::get()->getEventBase();
        // TODO: use a connection pool
        const folly::AsyncSocket::OptionMap opts {
            {{SOL_SOCKET, SO_REUSEADDR}, 1}
        };
        connector_.connect(evb, addr, std::chrono::milliseconds(60000), opts);
    }
    void ProxyHandler::onBody(std::unique_ptr<folly::IOBuf> body) noexcept {}
    void ProxyHandler::onUpgrade(UpgradeProtocol protocol) noexcept {}

    // Called when the client sends their EOM to the masternode
    void ProxyHandler::onEOM() noexcept {
        if (originTxn_) {
            // Forward the client's EOM to the origin server
            originTxn_->sendEOM();
        }
    }
    void ProxyHandler::requestComplete() noexcept { delete this; }
    void ProxyHandler::onError(ProxygenError err) noexcept { delete this; }

    // HTTPConnector::Callback methods

    // Called when the masternode connects to an origin server
    void ProxyHandler::connectSuccess(proxygen::HTTPUpstreamSession* session) noexcept {
        originTxn_ = session->newTransaction(&originHandler_);
        originTxn_->sendHeaders(*request_);
        downstream_->resumeIngress();
    }

    // Called when the masternode fails to connect to an origin server
    void ProxyHandler::connectError(const folly::AsyncSocketException& ex) noexcept {
        ResponseBuilder(downstream_)
            .status(502, "Bad Gateway")
            .sendWithEOM();
    }

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
