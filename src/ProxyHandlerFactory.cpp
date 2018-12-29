#include "ProxyHandlerFactory.h"
#include "DirectHandler.h"

void ProxyHandlerFactory::onServerStart(folly::EventBase *evb) noexcept {
    timer_->timer = HHWheelTimer::newTimer(
        evb,
        std::chrono::milliseconds(HHWheelTimer::DEFAULT_TICK_INTERVAL),
        folly::AsyncTimeout::InternalEnum::NORMAL,
        std::chrono::seconds(60000)); // todo: use config timeout

        LOG(INFO) << "Server thread now started and listening for requests!\n";
}

void ProxyHandlerFactory::onServerStop() noexcept {
    timer_->timer.reset();
    LOG(INFO) << "Server stopped\n";
}

RequestHandler* ProxyHandlerFactory::onRequest(RequestHandler *, HTTPMessage *m) noexcept {
    if (m->getHeaders().rawExists(DIRECT_HEADER_NAME)) {
        // Reply using masternode direct access handler
        return new DirectHandler(cache_.get(), config_.get(), state_.get());
    }
    return new ProxyHandler(timer_->timer.get(), cache_.get(), config_.get());
}
