#pragma once

#include "Config.h"
#include "ContentCache.h"
#include "NetworkState.h"
#include "ServiceWorker.h"

#include <proxygen/httpserver/HTTPServer.h>

namespace masternode {
    class Masternode {
        private:
            // Proxygen server to handle requests for content
            std::unique_ptr<proxygen::HTTPServer> server_{nullptr};
            // IP configurations to bind to
            std::vector<proxygen::HTTPServer::IPConfig> IPs_;
            // Configuration class
            std::shared_ptr<Config> config_{nullptr};
            // Keeps track of the pool's p2p network
            std::shared_ptr<NetworkState> state_{nullptr};
            // Stores cached web content
            std::shared_ptr<ContentCache> cache_{nullptr};
            // Manages the service worker implementation
            std::shared_ptr<ServiceWorker> sw_{nullptr};
        public:
            explicit Masternode(std::shared_ptr<Config>);
            void start(std::function<void()> onSuccess = nullptr,
                std::function<void(std::exception_ptr)> onError = nullptr);
            void stop();
    };
}
