#include "ProxyHandler.h"

#include <iostream>

#include <folly/io/async/EventBaseManager.h>

#include <proxygen/httpserver/RequestHandler.h>
#include <proxygen/httpserver/ResponseBuilder.h>
#include <proxygen/lib/http/session/HTTPUpstreamSession.h>
#include <proxygen/lib/utils/URL.h>

using namespace proxygen;

namespace masternode {
    ProxyHandler::ProxyHandler(folly::CPUThreadPoolExecutor *cpuPool,
        folly::HHWheelTimer *timer,
        MemoryCache *cache):
            cpuPool_(cpuPool),
            connector_{this, timer},
            originHandler_(*this),
            cache_(cache){}

    ProxyHandler::~ProxyHandler() {}
    
    // RequestHandler methods
    void ProxyHandler::onRequest(std::unique_ptr<HTTPMessage> headers) noexcept {
        request_ = std::move(headers);
        proxygen::URL url(request_->getURL());

        // check the cache for this url
        auto cachedRoute = cache_->getCachedRoute(url.getUrl());
        // if we have it cached, reply to client
        if (cachedRoute) {
            std::cout << "served request from cache!!!!!\n";
            ResponseBuilder(downstream_)
                .status(200, "OK")
                .header("Content-Type", cachedRoute.get()->getHeaders().get()->getHeaders().rawGet("Content-Type"))
                .header("Content-Encoding", cachedRoute.get()->getHeaders().get()->getHeaders().rawGet("Content-Encoding"))
                .body(cachedRoute.get()->getContent())
                .sendWithEOM();
            return;
        }
        // otherwise, connect to origin server to fetch content

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

        // Make a connection to the origin server
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
    void ProxyHandler::requestComplete() noexcept {
        // If we stored new content to cache
        if (contentBody_ && contentHeaders_) {
            proxygen::URL url(request_->getURL()); 
            cache_->addCachedRoute(url.getUrl(), contentBody_.get()->clone(), contentHeaders_); // may want to do this asynchronously
        }
        
        delete this;
    }
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

    /////////////////////////////////////////////////////////////
    // Start: HTTPTransactionHandler delegated methods

    void ProxyHandler::originSetTransaction(proxygen::HTTPTransaction* txn) noexcept {
        originTxn_ = txn;
    }

    void ProxyHandler::originDetachTransaction() noexcept {
        originTxn_ = nullptr;
        // TODO: add code to check for conditions to delete this handler here
    }

    void ProxyHandler::originOnHeadersComplete(std::unique_ptr<proxygen::HTTPMessage> msg) noexcept {
        downstream_->sendHeaders(*(msg.get()));
        contentHeaders_ = std::move(msg);
    }

    // Called when the masternode receives body content from the origin server
    // (can be called multiple times for one request as content comes through)
    void ProxyHandler::originOnBody(std::unique_ptr<folly::IOBuf> chain) noexcept {
        proxygen::URL url(request_->getURL());
        // If we've already received some body content
        if (contentBody_) {
            contentBody_->prependChain(chain.get()->clone());
        } else {
            contentBody_ = chain.get()->clone();
        }
        
        downstream_->sendBody(std::move(chain));
    }

    void ProxyHandler::originOnChunkHeader(size_t length) noexcept {
        downstream_->sendChunkHeader(length);
    }

    void ProxyHandler::originOnChunkComplete() noexcept {
        downstream_->sendChunkTerminator();
    }

    void ProxyHandler::originOnTrailers(std::unique_ptr<proxygen::HTTPHeaders> trailers) noexcept {}

    void ProxyHandler::originOnEOM() noexcept {
        downstream_->sendEOM();
    }

    void ProxyHandler::originOnUpgrade(proxygen::UpgradeProtocol protocol) noexcept {}
    void ProxyHandler::originOnError(const proxygen::HTTPException& error) noexcept {}

    void ProxyHandler::originOnEgressPaused() noexcept {
        originTxn_->pauseIngress();
    }

    void ProxyHandler::originOnEgressResumed() noexcept {
        originTxn_->resumeIngress();
    }

    void ProxyHandler::originOnPushedTransaction(proxygen::HTTPTransaction *txn) noexcept {}
    // End: HTTPTransactionHandler delegated methods
    /////////////////////////////////////////////////////////////
}
