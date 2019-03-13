#include "ProxyHandler.h"

#include <iostream>

#include <folly/io/async/EventBaseManager.h>

#include <proxygen/httpserver/ResponseBuilder.h>
#include <proxygen/lib/http/session/HTTPUpstreamSession.h>
#include <proxygen/lib/utils/URL.h>

using namespace proxygen;

ProxyHandler::ProxyHandler(folly::HHWheelTimer *timer,
    std::shared_ptr<ContentCache> cache, 
    std::shared_ptr<MasternodeConfig> config,
    std::shared_ptr<ServiceWorker> sw):
        connector_{this, timer},
        originHandler_(*this),
        cache_(cache),
        config_(config),
        sw_(sw) {}

// RequestHandler methods
void ProxyHandler::onRequest(std::unique_ptr<HTTPMessage> headers) noexcept {
    LOG(INFO) << "Received new request from " << headers->getClientIP();
    request_ = std::move(headers);
    proxygen::URL url(request_->getURL());

    // check the cache for this url
    const auto& cachedRoute = cache_->getCachedRoute(url.getUrl());
    
    // if we have it cached, reply to client
    if (cachedRoute) {
        LOG(INFO) << "Serving from cache for " << url.getUrl();
        if (config_->enableServiceWorker &&
            cachedRoute->getHeaders()->getHeaders().rawGet("Content-Type")
            .find("text/html") != std::string::npos) {
            // inject service worker bootstrap into <head> tag
            folly::fbstring injected_body = 
                sw_->injectServiceWorker(*cachedRoute->getContent());
            if (!injected_body.empty()) {
                ResponseBuilder(downstream_)
                    .status(200, "OK")
                    .header("Content-Type", cachedRoute->getHeaders()->
                        getHeaders().rawGet("Content-Type"))
                    .body(injected_body)
                    .sendWithEOM();
                return;
            }
        }
        
        ResponseBuilder(downstream_)
            .status(200, "OK")
            .header("Content-Type", cachedRoute->getHeaders()->
                getHeaders().rawGet("Content-Type"))
            .body(cachedRoute->getContent())
            .sendWithEOM();
        return;
    }
    // otherwise, connect to origin server to fetch content
    request_->stripPerHopHeaders();
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
    LOG(INFO) << "Connecting to origin server...";
    connector_.connect(evb, addr, std::chrono::milliseconds(60000), opts);
}

void ProxyHandler::onBody(std::unique_ptr<folly::IOBuf> body) noexcept {}
void ProxyHandler::onUpgrade(UpgradeProtocol protocol) noexcept {}

// Called when the client sends their EOM to the masternode
void ProxyHandler::onEOM() noexcept {
    LOG(INFO) << "Client sent EOM";
    if (originTxn_) {
        // Forward the client's EOM to the origin server
        originTxn_->sendEOM();
        LOG(INFO) << "Sending EOM to origin";
    }
}
void ProxyHandler::requestComplete() noexcept {
    LOG(INFO) << "Completed request";
    // If we stored new content to cache
    if (contentBody_ && contentHeaders_) {
        proxygen::URL url(request_->getURL());
        LOG(INFO) << "Adding " << url.getUrl() << " to memory cache";
        // todo: may want to do this asynchronously
        cache_->addCachedRoute(url.getUrl(),
            contentBody_->cloneCoalesced(), contentHeaders_); 
    }

    delete this;
}
void ProxyHandler::onError(ProxygenError err) noexcept {
    LOG(ERROR) << "Request Handler encounted an error:\n" 
        << getErrorString(err);
    delete this; 
}

// HTTPConnector::Callback methods

// Called when the masternode connects to an origin server
void ProxyHandler::connectSuccess(
    proxygen::HTTPUpstreamSession* session) noexcept {
    LOG(INFO) << "Connected to origin server";
    originTxn_ = session->newTransaction(&originHandler_);

    // strip compression headers so that we receive an uncompressed
    // response
    bool removed = request_->getHeaders().remove(HTTP_HEADER_ACCEPT_ENCODING);
    if (removed) LOG(INFO) << 
        "Stripped Accept-Encoding header from origin request";

    originTxn_->sendHeaders(*request_);
    LOG(INFO) << "Sent headers to origin server";
    downstream_->resumeIngress();
}

// Called when the masternode fails to connect to an origin server
void ProxyHandler::connectError(
    const folly::AsyncSocketException& ex) noexcept {
    ResponseBuilder(downstream_)
        .status(502, "Bad Gateway")
        .sendWithEOM();
    LOG(ERROR) << "Encountered an error when connecting to the origin server: "
        << ex.what();
}

/////////////////////////////////////////////////////////////
// Start: HTTPTransactionHandler delegated methods

void ProxyHandler::originSetTransaction(
    proxygen::HTTPTransaction* txn) noexcept {
    originTxn_ = txn;
    LOG(INFO) << "Set the origin transaction in request handler";
}

void ProxyHandler::originDetachTransaction() noexcept {
    originTxn_ = nullptr;
    ResponseBuilder(downstream_)
        .status(200, "OK")
        .header("Content-Type", contentHeaders_->
            getHeaders().rawGet("Content-Type"))
        .body(contentBody_->clone())
        .sendWithEOM();
    
    LOG(INFO) << "Detached origin transaction";
    // TODO: add code to check for conditions to delete this handler here
}

void ProxyHandler::originOnHeadersComplete(
    std::unique_ptr<proxygen::HTTPMessage> msg) noexcept {
    contentHeaders_ = std::move(msg);
}

// Called when the masternode receives body content from the origin server
// (can be called multiple times for one request as content comes through)
void ProxyHandler::originOnBody(
    std::unique_ptr<folly::IOBuf> chain) noexcept {
    // If we've already received some body content
    if (contentBody_) {
        contentBody_->prependChain(chain->clone());
    } else {
        contentBody_ = chain->clone();
    }
}

void ProxyHandler::originOnChunkHeader(size_t length) noexcept {
    LOG(INFO) << "originOnChunkHeader()";
}

void ProxyHandler::originOnChunkComplete() noexcept {
    LOG(INFO) << "originOnChunkComplete()";
}

void ProxyHandler::originOnTrailers(
    std::unique_ptr<proxygen::HTTPHeaders> trailers) noexcept {
    LOG(INFO) << "originOnTrailers()";
}

void ProxyHandler::originOnEOM() noexcept {
    LOG(INFO) << "Sent EOM from origin to client";
}

void ProxyHandler::originOnUpgrade(
    proxygen::UpgradeProtocol protocol) noexcept {}
void ProxyHandler::originOnError(
    const proxygen::HTTPException& error) noexcept {
    LOG(INFO) << "Received error from origin: " << error.describe();
}

void ProxyHandler::originOnEgressPaused() noexcept {
    originTxn_->pauseIngress();
}

void ProxyHandler::originOnEgressResumed() noexcept {
    originTxn_->resumeIngress();
}

void ProxyHandler::originOnPushedTransaction(
    proxygen::HTTPTransaction *txn) noexcept {}
// End: HTTPTransactionHandler delegated methods
/////////////////////////////////////////////////////////////
