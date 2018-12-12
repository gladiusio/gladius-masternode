#pragma once

#include <unistd.h>

#include <folly/io/async/EventBaseManager.h>

#include <proxygen/httpserver/RequestHandlerFactory.h>

#include "Cache.h"
#include "ProxyHandler.h"

using namespace proxygen;
using folly::HHWheelTimer;

namespace masternode {

    class ProxyHandlerFactory : public RequestHandlerFactory {
        public:
            ProxyHandlerFactory():
                cache_(std::make_shared<MemoryCache>(0)) {
                LOG(INFO) << "Constructing a new ProxyHandlerFactory\n";
            };
            ~ProxyHandlerFactory() {
                LOG(INFO) << "Destroying a ProxyHandlerFactory\n";
            };
            void onServerStart(folly::EventBase *) noexcept override;
            void onServerStop() noexcept override;
            RequestHandler *onRequest(RequestHandler *, HTTPMessage *) noexcept override;
        protected:
            std::shared_ptr<MemoryCache> cache_;
        private:
            struct TimerWrapper {
                HHWheelTimer::UniquePtr timer;
            };
            folly::ThreadLocal<TimerWrapper> timer_;
    };
}
