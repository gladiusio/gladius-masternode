#include "Router.h"
#include "DirectHandler.h"
#include "RedirectHandler.h"
#include "ProxyHandler.h"
#include "ServiceWorkerHandler.h"

Router::Router(std::shared_ptr<MasternodeConfig>& config,
    std::shared_ptr<NetworkState>& state,
    std::shared_ptr<ContentCache>& cache,
    std::shared_ptr<ServiceWorker>& sw):
    cache_(cache),
    config_(config),
    state_(state),
    sw_(sw) {}

Router::~Router() {}

void Router::onServerStart(folly::EventBase *evb) noexcept {
    timer_->timer = HHWheelTimer::newTimer(
        evb,
        std::chrono::milliseconds(HHWheelTimer::DEFAULT_TICK_INTERVAL),
        folly::AsyncTimeout::InternalEnum::NORMAL,
        std::chrono::seconds(60000)); // todo: use config timeout
    
    LOG(INFO) << "Server thread now started and listening for requests!\n";
}

void Router::onServerStop() noexcept {
    timer_->timer.reset();
    LOG(INFO) << "Server stopped\n";
}

RequestHandler* Router::onRequest(RequestHandler *req, HTTPMessage *m) noexcept {
    // make sure this request always has a Host: header
    m->ensureHostHeader();
    
    // logic to determine which handler to create based on the request

    // if upgrading insecure requests is enabled and this request is
    // not secure, redirect to the secure port
    if (config_->upgrade_insecure && !m->isSecure()) {
        return new RedirectHandler(config_.get());
    }

    // For speaking directly to the masternode and not an underlying
    // (protected) domain
    if (m->getHeaders().rawExists(DIRECT_HEADER_NAME)) {
        return new DirectHandler(cache_.get(), config_.get(), state_.get());
    }

    // serving the service worker javascript file itself
    if (m->getURL() == "/gladius-service-worker.js") {
        return new ServiceWorkerHandler(config_.get(), sw_.get());
    }

    // all other requests for proxied content
    return new ProxyHandler(timer_->timer.get(), cache_.get(), config_.get(), sw_.get());
}
