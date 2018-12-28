#pragma once

#include <string.h>
#include <stdint.h>

#include "MasternodeConfig.h"
#include "NetworkState.h"

#include <proxygen/httpserver/HTTPServer.h>

namespace masternode {
    class Masternode {
        private:
            std::shared_ptr<MasternodeConfig> config_{nullptr};
            std::unique_ptr<proxygen::HTTPServer> server_{nullptr};
            std::shared_ptr<NetworkState> state_{nullptr};
        public:
            Masternode(std::shared_ptr<MasternodeConfig> config) {
                config_ = config;
                server_ = std::make_unique<proxygen::HTTPServer>(
                    std::move(config_->options));
                if (!config_->gateway_address.empty()) {
                    state_ = std::make_shared<NetworkState>(config_);
                }
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
    };
}
