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

bool ProxyHandler::checkForShutdown() {
    if (clientTerminated_ && !originTxn_) {
        delete this;
        return true;
    }
    return false;
}

void ProxyHandler::abortDownstream() {
    if (!clientTerminated_) {
        downstream_->sendAbort();
    }
}

// RequestHandler methods
void ProxyHandler::onRequest(std::unique_ptr<HTTPMessage> headers) noexcept {
    request_ = std::move(headers);
    proxygen::URL url(request_->getURL());

    if (request_->getMethod() == HTTPMethod::GET) {
        // check the cache for this url
        const auto& cachedRoute = cache_->getCachedRoute(url.getUrl());
        
        // if we have it cached, reply to client
        if (cachedRoute) {
            std::unique_ptr<folly::IOBuf> content = cachedRoute->getContent();
            VLOG(1) << "Serving from cache for " << url.getUrl();
            if (config_->enableServiceWorker &&
                cachedRoute->getHeaders()->getHeaders().rawGet("Content-Type")
                .find("text/html") != std::string::npos) {
                // inject service worker bootstrap into <head> tag
                auto injected_body = 
                    sw_->injectServiceWorker(*cachedRoute->getContent());
                if (!injected_body.empty()) {
                    content = folly::IOBuf::copyBuffer(injected_body);
                }
            }
            
            ResponseBuilder(downstream_)
                .status(200, "OK")
                .header("Content-Type", cachedRoute->getHeaders()->
                    getHeaders().getSingleOrEmpty(HTTP_HEADER_CONTENT_TYPE))
                .header("Cache-Control", cachedRoute->getHeaders()->
                    getHeaders().getSingleOrEmpty(HTTP_HEADER_CACHE_CONTROL))
                .header("ETag", cachedRoute->getHeaders()->
                    getHeaders().getSingleOrEmpty(HTTP_HEADER_ETAG))
                .header("Expires", cachedRoute->getHeaders()->
                    getHeaders().getSingleOrEmpty(HTTP_HEADER_EXPIRES))
                .header("Last-Modified", cachedRoute->getHeaders()->
                    getHeaders().getSingleOrEmpty(HTTP_HEADER_LAST_MODIFIED))
                .body(content->clone())
                .sendWithEOM();
            return;
        }
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
    VLOG(1) << "Connecting to origin server...";
    connector_.connect(evb, addr, std::chrono::milliseconds(60000), opts);
}

void ProxyHandler::onBody(std::unique_ptr<folly::IOBuf> body) noexcept {
    int bytes = ((body) ? body->computeChainDataLength() : 0);
    if (originTxn_) {
        VLOG(1) << "Forwarding " << bytes << " body bytes to origin";
        originTxn_->sendBody(std::move(body));
    } else {
        VLOG(1) << "Dropping " << bytes << 
            " body bytes from client to origin";
    }
}
void ProxyHandler::onUpgrade(UpgradeProtocol protocol) noexcept {}

// Called when the client sends their EOM to the masternode
void ProxyHandler::onEOM() noexcept {
    VLOG(1) << "Client sent EOM";
    if (originTxn_) {
        // Forward the client's EOM to the origin server
        originTxn_->sendEOM();
        VLOG(1) << "Sending EOM to origin";
    } else {
        VLOG(1) << "Dropping client EOM to origin";
    }
}
void ProxyHandler::requestComplete() noexcept {
    VLOG(1) << "Completed request";
    // If we stored new content to cache
    if (request_->getMethod() == HTTPMethod::GET &&
        contentBody_ && contentHeaders_) {
        proxygen::URL url(request_->getURL());
        VLOG(1) << "Adding " << url.getUrl() << " to memory cache";
        // todo: may want to do this asynchronously
        cache_->addCachedRoute(url.getUrl(),
            contentBody_->cloneCoalesced(), contentHeaders_); 
    }

    clientTerminated_ = true;
    checkForShutdown();
}
void ProxyHandler::onError(ProxygenError err) noexcept {
    LOG(ERROR) << "Proxy Request Handler encountered a client error:" 
        << getErrorString(err);
    clientTerminated_ = true;
    if (originTxn_) {
        originTxn_->sendAbort();
    }
    checkForShutdown();
}

// HTTPConnector::Callback methods

// Called when the masternode connects to an origin server
void ProxyHandler::connectSuccess(
    proxygen::HTTPUpstreamSession* session) noexcept {
    VLOG(1) << "Connected to origin server";
    originTxn_ = session->newTransaction(&originHandler_);

    // strip compression headers so that we receive an uncompressed
    // response
    bool removed = request_->getHeaders().remove(HTTP_HEADER_ACCEPT_ENCODING);
    if (removed) VLOG(1) << 
        "Stripped Accept-Encoding header from origin request";

    originTxn_->sendHeaders(*request_);
    VLOG(1) << "Sent headers to origin server";
    downstream_->resumeIngress();
}

// Called when the masternode fails to connect to an origin server
void ProxyHandler::connectError(
    const folly::AsyncSocketException& ex) noexcept {
    LOG(ERROR) << "Encountered an error when connecting to the origin server: "
        << ex.what();
    if (!clientTerminated_) {
        ResponseBuilder(downstream_)
            .status(503, "Bad Gateway")
            .sendWithEOM();
    } else {
        abortDownstream();
        checkForShutdown();
    }
}

/////////////////////////////////////////////////////////////
// Start: HTTPTransactionHandler delegated methods

void ProxyHandler::originSetTransaction(
    proxygen::HTTPTransaction* txn) noexcept {
    originTxn_ = txn;
    VLOG(1) << "Set the origin transaction in request handler";
}

void ProxyHandler::originDetachTransaction() noexcept {
    VLOG(1) << "Detached origin transaction";
    originTxn_ = nullptr;
    checkForShutdown();
}

void ProxyHandler::originOnHeadersComplete(
    std::unique_ptr<proxygen::HTTPMessage> msg) noexcept {
    if (clientTerminated_) return;
    contentHeaders_ = std::move(msg);
}

// Called when the masternode receives body content from the origin server
// (can be called multiple times for one request as content comes through)
void ProxyHandler::originOnBody(
    std::unique_ptr<folly::IOBuf> chain) noexcept {
    if (clientTerminated_) return;
    // If we've already received some body content
    if (contentBody_) {
        contentBody_->prependChain(chain->clone());
    } else {
        contentBody_ = chain->clone();
    }
}

void ProxyHandler::originOnChunkHeader(size_t length) noexcept {
    VLOG(1) << "originOnChunkHeader()";
}

void ProxyHandler::originOnChunkComplete() noexcept {
    VLOG(1) << "originOnChunkComplete()";
}

void ProxyHandler::originOnTrailers(
    std::unique_ptr<proxygen::HTTPHeaders> trailers) noexcept {
    VLOG(1) << "originOnTrailers()";
}

void ProxyHandler::originOnEOM() noexcept {
    // check that there's still a connection and body/headers
    // to be able to send back to the client
    if (!clientTerminated_ && contentHeaders_ && contentBody_){
        ResponseBuilder(downstream_)
            .status(200, "OK")
            .header("Content-Type", contentHeaders_->
                getHeaders().rawGet("Content-Type"))
            .body(contentBody_->clone())
            .sendWithEOM();
    }
}

void ProxyHandler::originOnUpgrade(
    proxygen::UpgradeProtocol protocol) noexcept {}

void ProxyHandler::originOnError(
    const proxygen::HTTPException& error) noexcept {
    LOG(ERROR) << "Received error from origin: " << error.describe();
    // todo: send an error back to the client and set condtions so
    // request handling doesn't proceed
    abortDownstream();
}

void ProxyHandler::originOnEgressPaused() noexcept {
    if (!clientTerminated_ && originTxn_) {
        originTxn_->pauseIngress();
    }
}

void ProxyHandler::originOnEgressResumed() noexcept {
    if (!clientTerminated_ && originTxn_) {
        originTxn_->resumeIngress();
    }
}

void ProxyHandler::originOnPushedTransaction(
    proxygen::HTTPTransaction *txn) noexcept {}
// End: HTTPTransactionHandler delegated methods
/////////////////////////////////////////////////////////////
