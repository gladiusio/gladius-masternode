#pragma once

#include <string.h>
#include <stdint.h>

#include "MasternodeConfig.h"
#include "NetworkState.h"
#include "ProxyHandlerFactory.h"

#include <proxygen/httpserver/HTTPServer.h>


namespace masternode {
    class Masternode {
        private:
            // Proxygen server to handle requests for content
            std::unique_ptr<proxygen::HTTPServer> server_{nullptr};
            // Configuration class
            std::shared_ptr<MasternodeConfig> config_{nullptr};
            // Keeps track of the pool's p2p network
            std::shared_ptr<NetworkState> state_{nullptr};
            // Stores cached web content
            std::shared_ptr<ContentCache> cache_{nullptr};
        public:
            Masternode(std::shared_ptr<MasternodeConfig>);
            void start(std::function<void()> onSuccess = nullptr,
                std::function<void(std::exception_ptr)> onError = nullptr);
            void stop();

            std::shared_ptr<NetworkState> getNetworkState();
    };
}
