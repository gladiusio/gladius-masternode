#pragma once

#include "Cache.h"
#include "MasternodeConfig.h"
#include "NetworkState.h"

#include <proxygen/httpserver/RequestHandler.h>

class DirectHandler : public proxygen::RequestHandler {
    public:
        DirectHandler(std::shared_ptr<ContentCache>, 
            std::shared_ptr<MasternodeConfig>, 
            std::shared_ptr<NetworkState>);

        // RequestHandler methods
        void onRequest(std::unique_ptr<proxygen::HTTPMessage> headers) noexcept override;
        void onBody(std::unique_ptr<folly::IOBuf> body) noexcept override;
        void onUpgrade(proxygen::UpgradeProtocol protocol) noexcept override;
        void onEOM() noexcept override;
        void requestComplete() noexcept override;
        void onError(proxygen::ProxygenError err) noexcept override;
    private:
        // HTTP content cache
        std::shared_ptr<ContentCache> cache_{nullptr};

        // Configuration class
        std::shared_ptr<MasternodeConfig> config_{nullptr};

        // Network state
        std::shared_ptr<NetworkState> state_{nullptr};
};
