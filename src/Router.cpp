#include "Router.h"
#include "DirectHandler.h"
#include "RedirectHandler.h"
#include "ProxyHandler.h"
#include "ServiceWorkerHandler.h"
#include "RejectHandler.h"

Router::Router(std::shared_ptr<MasternodeConfig> config,
    std::shared_ptr<NetworkState> state,
    std::shared_ptr<ContentCache> cache,
    std::shared_ptr<ServiceWorker> sw):
    cache_(cache),
    config_(config),
    state_(state),
    sw_(sw) {
        CHECK(config_) << "Config object was null";
        CHECK(cache_) << "Cache object was null";
        VLOG(1) << "Router created";
    }

void Router::onServerStart(folly::EventBase *evb) noexcept {
    timer_->timer = HHWheelTimer::newTimer(
        evb,
        std::chrono::milliseconds(HHWheelTimer::DEFAULT_TICK_INTERVAL),
        folly::AsyncTimeout::InternalEnum::NORMAL,
        std::chrono::seconds(60000)); // todo: use config timeout
    
    LOG(INFO) << "Server thread now started and listening for requests!";
}

void Router::onServerStop() noexcept {
    timer_->timer.reset();
    LOG(INFO) << "Server thread stopped";
}

bool Router::requestIsValid(std::string host) {
    if (host == config_->protected_domain) return true;
    if (config_->ssl_enabled) {
        if (host == config_->protected_domain + ":" + std::to_string(config_->ssl_port)) return true;
    }
    if (host == config_->protected_domain + ":" + std::to_string(config_->port)) return true;
    return false;
}


RequestHandler* Router::onRequest(
    RequestHandler *req, HTTPMessage *m) noexcept {

    logRequest(m);
    // make sure this request always has a Host: header
    m->ensureHostHeader();
    
    std::string host = m->getHeaders().getSingleOrEmpty(HTTP_HEADER_HOST);

    if (!requestIsValid(host)) {
        // reject this request
        return new RejectHandler(400, "Bad Request");
    }
 
    
    // logic to determine which handler to create based on the request

    // if upgrading insecure requests is enabled and this request is
    // not secure, redirect to the secure port
    if (config_->upgrade_insecure && !m->isSecure()) {
        return new RedirectHandler(config_);
    }

    // For speaking directly to the masternode and not an underlying
    // (protected) domain
    if (m->getHeaders().rawExists(DIRECT_HEADER_NAME)) {
        return new DirectHandler(cache_, config_, state_);
    }

    if (config_->enableServiceWorker && sw_) {
        // serving the service worker javascript file itself
        if (m->getURL() == "/gladius-service-worker.js") {
            return new ServiceWorkerHandler(config_, sw_);
        }
    }

    // all other requests for proxied content
    return new ProxyHandler(timer_->timer.get(), cache_, config_, sw_);
}

void Router::logRequest(HTTPMessage *m) {
    // ip [time] method url protocol
    LOG(INFO) << m->getClientIP() << " " <<
        "[" << getDateTimeStr(m->getStartTime()) << "] " <<
        m->getMethodString() << " " << m->getURL();
}
