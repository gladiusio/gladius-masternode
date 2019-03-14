#pragma once

#include <unistd.h>

#include <folly/io/async/EventBaseManager.h>
#include <proxygen/httpserver/RequestHandlerFactory.h>

#include "NetworkState.h"
#include "Cache.h"
#include "ServiceWorker.h"

using namespace proxygen;
using folly::HHWheelTimer;

class Router : public RequestHandlerFactory {
    public:
        Router(std::shared_ptr<MasternodeConfig>,
            std::shared_ptr<NetworkState>,
            std::shared_ptr<ContentCache>,
            std::shared_ptr<ServiceWorker>);

        // Use this method to setup thread local data
        void onServerStart(folly::EventBase *) noexcept override;
        void onServerStop() noexcept override;
        RequestHandler *onRequest(RequestHandler *, HTTPMessage *)
            noexcept override;
    protected:
        std::shared_ptr<ContentCache> cache_{nullptr};
        std::shared_ptr<MasternodeConfig> config_{nullptr};
        std::shared_ptr<NetworkState> state_{nullptr};
        std::shared_ptr<ServiceWorker> sw_{nullptr};

        std::string DIRECT_HEADER_NAME = "Gladius-Masternode-Direct";
    private:
        void logRequest(HTTPMessage *m);
        struct TimerWrapper {
            HHWheelTimer::UniquePtr timer;
        };
        folly::ThreadLocal<TimerWrapper> timer_;
};
