#include "Router.h"
#include "DirectHandler.h"
#include "ProxyHandler.h"
#include "ServiceWorkerHandler.h"

Router::Router(std::shared_ptr<MasternodeConfig> config,
    std::shared_ptr<NetworkState> state,
    std::shared_ptr<ContentCache> cache):
    cache_(cache),
    config_(config),
    state_(state)
{
    // read the service worker file if it exists from disk 
}

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
    // logic to determine which handler to create based on the request

    if (m->getHeaders().rawExists(DIRECT_HEADER_NAME)) {
        return new DirectHandler(cache_.get(), config_.get(), state_.get());
    }

    if (m->getURL() == "/gladius-service-worker.js") {
        return new ServiceWorkerHandler(config_.get());
    }

    return new ProxyHandler(timer_->timer.get(), cache_.get(), config_.get());
}
