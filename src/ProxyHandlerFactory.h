#pragma once

#include <unistd.h>

#include <folly/io/async/EventBaseManager.h>

#include <proxygen/httpserver/RequestHandlerFactory.h>

#include "Cache.h"
#include "ProxyHandler.h"
#include "Masternode.h"

using namespace proxygen;
using folly::HHWheelTimer;

using namespace masternode;

class ProxyHandlerFactory : public RequestHandlerFactory {
    public:
        ProxyHandlerFactory(std::shared_ptr<MasternodeConfig> config):
            cache_(std::make_shared<MemoryCache>(0, config->cache_directory)),
            config_(config) {
            LOG(INFO) << "Constructing a new ProxyHandlerFactory\n";
        };
        ~ProxyHandlerFactory() {
            LOG(INFO) << "Destroying a ProxyHandlerFactory\n";
        };
        // Use this method to setup thread local data
        void onServerStart(folly::EventBase *) noexcept override;
        void onServerStop() noexcept override;
        RequestHandler *onRequest(RequestHandler *, HTTPMessage *) noexcept override;
    protected:
        std::shared_ptr<MemoryCache> cache_;
        std::shared_ptr<MasternodeConfig> config_;
        std::string DIRECT_HEADER_NAME = "Gladius-Masternode-Direct";
    private:
        struct TimerWrapper {
            HHWheelTimer::UniquePtr timer;
        };
        folly::ThreadLocal<TimerWrapper> timer_;
};
