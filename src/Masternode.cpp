#include "Masternode.h"
#include "Router.h"

using namespace proxygen;
using namespace folly::ssl;
using namespace masternode;

Masternode::Masternode(std::shared_ptr<Config> config):
    config_(config)
{
    CHECK(config) << "Config object was null";

    auto srvConf = config_->getServerConfig();
    auto ftConf = config_->getFeaturesConfig();
    auto domConf = config_->getProtectedDomainsConfig();
    auto sslConf = config_->getSSLConfig();

    // configure the NetworkState for peer to peer features
    if (ftConf.p2pConfig.enabled) {
        state_ = std::make_shared<NetworkState>(config_);
    }
    
    // configure the content cache
    cache_ = std::make_shared<ContentCache>(
        srvConf.cache.maxEntries,
        srvConf.cache.diskPath,
        ftConf.p2pConfig.enabled);

    // configure the service worker module
    if (ftConf.sw.enabled) {
        sw_ = std::make_shared<ServiceWorker>(ftConf.sw.path);
    }
    
    // Proxygen server options
    proxygen::HTTPServerOptions options;
    options.threads = srvConf.threads;
    options.idleTimeout = std::chrono::milliseconds(
        srvConf.idleTimeoutMs);
    options.shutdownOn = {SIGINT, SIGTERM};
    options.enableContentCompression = 
        ftConf.compressionConfig.enabled;
    if (options.enableContentCompression) {
        options.contentCompressionLevel = ftConf.compression.level;
        options.contentCompressionMinimumSize = ftConf.compression.minSize;
    }
    options.supportsConnect = false;
    options.handlerFactories = proxygen::RequestHandlerChain()
        .addThen<Router>(config_, state_, cache_, sw_)
        .build();

    // configure IP addresses and SSL
    IPs_.push_back({
        folly::SocketAddress(srvConf.ip, srvConf.port),
        HTTPServer::Protocol::HTTP
    });

    if (sslConf.enabled) {
        HTTPServer::IPConfig sslIP = {
            {folly::SocketAddress(srvConf.ip, sslConf.port)},
            HTTPServer::Protocol::HTTP
        };
        for (const auto& cert : sslConf.certs) {
            wangle::SSLContextConfig ctx;
            ctx.isDefault = cert.isDefault;
            ctx.clientVerification =
                folly::SSLContext::SSLVerifyPeerEnum::NO_VERIFY;
            ctx.setCertificate(cert.certPath, cert.keyPath, "");
            sslIP.sslConfigs.push_back(ctx);
        }
        IPs_.push_back(sslIP);
    }

    server_ = std::make_unique<proxygen::HTTPServer>(
        std::move(options));
}

void Masternode::start(std::function<void()> onSuccess,
    std::function<void(std::exception_ptr)> onError) {
    
    LOG(INFO) << "Starting masternode...";
    if (config_->getFeaturesConfig().p2pConfig.enabled && state_) {
        state_->beginPollingGateway();
    }
    server_->bind(IPs_);
    server_->start(onSuccess, onError);
}

void Masternode::stop() {
    LOG(INFO) << "Stopping masternode...";
    if (server_) {
        server_->stop();
    }
}
