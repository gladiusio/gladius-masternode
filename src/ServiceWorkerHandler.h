#pragma once

#include "MasternodeConfig.h"
#include "ServiceWorker.h"

#include <proxygen/httpserver/RequestHandler.h>

class ServiceWorkerHandler : public proxygen::RequestHandler {
    public:
        ServiceWorkerHandler(std::shared_ptr<MasternodeConfig>,
            std::shared_ptr<ServiceWorker>);

        // RequestHandler methods
        void onRequest(std::unique_ptr<proxygen::HTTPMessage> headers) noexcept override;
        void onBody(std::unique_ptr<folly::IOBuf> body) noexcept override;
        void onUpgrade(proxygen::UpgradeProtocol protocol) noexcept override;
        void onEOM() noexcept override;
        void requestComplete() noexcept override;
        void onError(proxygen::ProxygenError err) noexcept override;
    private:
        // Configuration class
        std::shared_ptr<MasternodeConfig> config_{nullptr};
        // Service worker wrapper
        std::shared_ptr<ServiceWorker> sw_{nullptr};
};
