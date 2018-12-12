#include "ProxyHandlerFactory.h"

using namespace masternode;

void ProxyHandlerFactory::onServerStart(folly::EventBase *evb) noexcept {
    timer_->timer = HHWheelTimer::newTimer(
        evb,
        std::chrono::milliseconds(HHWheelTimer::DEFAULT_TICK_INTERVAL),
        folly::AsyncTimeout::InternalEnum::NORMAL,
        std::chrono::seconds(60000));

        LOG(INFO) << "Server thread now started and listening for requests!\n";
}

void ProxyHandlerFactory::onServerStop() noexcept {
    timer_->timer.reset();
    LOG(INFO) << "Server stopped\n";
}

RequestHandler* ProxyHandlerFactory::onRequest(RequestHandler *, HTTPMessage *) noexcept {
    return new ProxyHandler(timer_->timer.get(), cache_.get());
}
