#include "Masternode.h"
#include "ProxyHandlerFactory.h"

using namespace masternode;

Masternode::Masternode(std::shared_ptr<MasternodeConfig> config) {
    config_ = config;
    state_ = std::make_shared<NetworkState>(config_);
    cache_ = std::make_shared<ContentCache>(256, config_->cache_directory);
    config_->options.handlerFactories = proxygen::RequestHandlerChain()
        .addThen<ProxyHandlerFactory>(config_, state_, cache_)
        .build();

    server_ = std::make_unique<proxygen::HTTPServer>(
        std::move(config_->options));
}

void Masternode::start(std::function<void()> onSuccess,
    std::function<void(std::exception_ptr)> onError) {
    if (state_) {
        state_->beginPollingGateway();
    }
    server_->bind(config_->IPs);
    server_->start(onSuccess, onError);
}

void Masternode::stop() {
    if (server_.get()) {
        server_->stop();
    }
}

std::shared_ptr<NetworkState> Masternode::getNetworkState() { return state_; }
