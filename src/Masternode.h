#pragma once

#include <string.h>
#include <stdint.h>

#include <boost/thread.hpp>
#include <proxygen/httpserver/HTTPServer.h>

namespace masternode {
    class MasternodeConfig {
        public:
            // IP or hostname of the origin server (required)
            std::string origin_host;
            // Port of the origin server (required)
            uint16_t origin_port;
            // IP or hostname of the host we're protecting (singular for now) (required)
            std::string protected_host;
            // Proxygen server options
            proxygen::HTTPServerOptions options;
            // IPs for the server to locally bind to
            std::vector<proxygen::HTTPServer::IPConfig> IPs;
            // Directory to store cached files
            std::string cache_directory;
    };

    class Masternode {
        private:
            std::shared_ptr<MasternodeConfig> config_;
            std::unique_ptr<proxygen::HTTPServer> server_;
            std::shared_ptr<folly::CPUThreadPoolExecutor> cpuThreadPool_;
        public:
            Masternode(std::shared_ptr<MasternodeConfig> config) {
                config_ = config;
                cpuThreadPool_ = std::make_shared<folly::CPUThreadPoolExecutor>(
                    config_->options.threads,
                    std::make_shared<folly::NamedThreadFactory>("CpuAsyncThread"));
                server_ = std::make_unique<proxygen::HTTPServer>(std::move(config_->options));
            };
            
            void start(std::function<void()> onSuccess = nullptr,
                std::function<void(std::exception_ptr)> onError = nullptr) {
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
