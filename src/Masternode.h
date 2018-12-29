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
            Masternode(std::shared_ptr<MasternodeConfig> config) {
                config_ = config;
                state_ = std::make_shared<NetworkState>(config_);
                cache_ = std::make_shared<ContentCache>(256, config_->cache_directory);
                config_->options.handlerFactories = proxygen::RequestHandlerChain()
                    .addThen<ProxyHandlerFactory>(config_, state_, cache_)
                    .build();

                server_ = std::make_unique<proxygen::HTTPServer>(
                    std::move(config_->options));
            };
            
            void start(std::function<void()> onSuccess = nullptr,
                std::function<void(std::exception_ptr)> onError = nullptr) {
                if (state_) {
                    state_->beginPollingGateway();
                }
                server_->bind(config_->IPs);
                server_->start(onSuccess, onError);
            }

            void stop() {
                if (server_.get()) {
                    server_->stop();
                }
            }

            std::shared_ptr<NetworkState> getNetworkState() { return state_; }
    };
}
