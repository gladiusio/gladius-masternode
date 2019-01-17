#include "ProxyHandler.h"

#include <iostream>

#include <folly/io/async/EventBaseManager.h>

#include <proxygen/httpserver/ResponseBuilder.h>
#include <proxygen/lib/http/session/HTTPUpstreamSession.h>
#include <proxygen/lib/utils/URL.h>

using namespace proxygen;

ProxyHandler::ProxyHandler(folly::HHWheelTimer *timer,
    ContentCache *cache, MasternodeConfig *config, ServiceWorker *sw):
        connector_{this, timer},
        originHandler_(*this),
        cache_(cache),
        config_(config){}

ProxyHandler::~ProxyHandler() {}

// RequestHandler methods
void ProxyHandler::onRequest(std::unique_ptr<HTTPMessage> headers) noexcept {
    LOG(INFO) << "Received new request from " << headers->getClientIP() << "\n";
    request_ = std::move(headers);
    proxygen::URL url(request_->getURL());

    // check the cache for this url
    auto cachedRoute = cache_->getCachedRoute(url.getUrl());
    // if we have it cached, reply to client
    if (cachedRoute) {
        if (cachedRoute->getHeaders()->getHeaders().rawGet("Content-Type")
            .find("text/html") != std::string::npos) {
                // inject service worker bootstrap into <head> tag
                // todo: use MyHTML lib https://github.com/lexborisov/myhtml/blob/master/INSTALL.md
            }
        LOG(INFO) << "Serving from cache for " << url.getUrl() << "\n";
        ResponseBuilder(downstream_)
            .status(200, "OK")
            .header("Content-Type", cachedRoute->getHeaders()->getHeaders().rawGet("Content-Type"))
            .header("Content-Encoding", cachedRoute->getHeaders()->getHeaders().rawGet("Content-Encoding"))
            .body(cachedRoute->getContent())
            .sendWithEOM();
        return;
    }
    // otherwise, connect to origin server to fetch content

    folly::SocketAddress addr;
    try {
        addr.setFromHostPort(config_->origin_host, config_->origin_port);
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
    LOG(INFO) << "Connecting to origin server...\n";
    connector_.connect(evb, addr, std::chrono::milliseconds(60000), opts);
}
void ProxyHandler::onBody(std::unique_ptr<folly::IOBuf> body) noexcept {}
void ProxyHandler::onUpgrade(UpgradeProtocol protocol) noexcept {}

// Called when the client sends their EOM to the masternode
void ProxyHandler::onEOM() noexcept {
    LOG(INFO) << "Client sent EOM\n";
    if (originTxn_) {
        // Forward the client's EOM to the origin server
        originTxn_->sendEOM();
        LOG(INFO) << "Sending EOM to origin\n";
    }
}
void ProxyHandler::requestComplete() noexcept {
    LOG(INFO) << "Completed request\n";
    // If we stored new content to cache
    if (contentBody_ && contentHeaders_) {
        proxygen::URL url(request_->getURL());
        LOG(INFO) << "Adding " << url.getUrl() << " to memory cache\n";
        cache_->addCachedRoute(url.getUrl(), contentBody_->clone(), contentHeaders_); // may want to do this asynchronously
    }
    LOG(INFO) << "Deleting RequestHandler now...\n";
    delete this;
}
void ProxyHandler::onError(ProxygenError err) noexcept {
    LOG(ERROR) << "Request Handler encounted an error:\n" << getErrorString(err) << "\n";
    delete this; 
}

// HTTPConnector::Callback methods

// Called when the masternode connects to an origin server
void ProxyHandler::connectSuccess(proxygen::HTTPUpstreamSession* session) noexcept {
    LOG(INFO) << "Connected to origin server\n";
    originTxn_ = session->newTransaction(&originHandler_);
    originTxn_->sendHeaders(*request_);
    LOG(INFO) << "Sent headers to origin server\n";
    downstream_->resumeIngress();
}

// Called when the masternode fails to connect to an origin server
void ProxyHandler::connectError(const folly::AsyncSocketException& ex) noexcept {
    ResponseBuilder(downstream_)
        .status(502, "Bad Gateway")
        .sendWithEOM();
    LOG(ERROR) << "Encountered an error when connecting to the origin server\n";
}

/////////////////////////////////////////////////////////////
// Start: HTTPTransactionHandler delegated methods

void ProxyHandler::originSetTransaction(proxygen::HTTPTransaction* txn) noexcept {
    originTxn_ = txn;
    LOG(INFO) << "Set the origin transaction in request handler\n";
}

void ProxyHandler::originDetachTransaction() noexcept {
    originTxn_ = nullptr;
    LOG(INFO) << "Detached origin transaction\n";
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
        contentBody_->prependChain(chain->clone());
    } else {
        contentBody_ = chain->clone();
    }
    
    downstream_->sendBody(std::move(chain));
    LOG(INFO) << "Sent body content from origin to client\n";
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
    LOG(INFO) << "Sent EOM from origin to client\n";
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
