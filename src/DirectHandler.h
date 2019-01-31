#pragma once

#include "Cache.h"
#include "MasternodeConfig.h"
#include "NetworkState.h"

#include <proxygen/httpserver/RequestHandler.h>

class DirectHandler : public proxygen::RequestHandler {
    public:
        DirectHandler(ContentCache *, MasternodeConfig *, NetworkState *);
        ~DirectHandler() override;

        // RequestHandler methods
        void onRequest(std::unique_ptr<proxygen::HTTPMessage> headers) noexcept override;
        void onBody(std::unique_ptr<folly::IOBuf> body) noexcept override;
        void onUpgrade(proxygen::UpgradeProtocol protocol) noexcept override;
        void onEOM() noexcept override;
        void requestComplete() noexcept override;
        void onError(proxygen::ProxygenError err) noexcept override;
    private:
        // HTTP content cache
        ContentCache *cache_{nullptr};

        // Configuration class
        MasternodeConfig *config_{nullptr};

        // Network state
        NetworkState *state_{nullptr};
};
