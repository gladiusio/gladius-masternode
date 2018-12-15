#pragma once

#include <string.h>
#include <stdint.h>

#include <boost/thread.hpp>
#include <proxygen/httpserver/HTTPServer.h>

#include "ProxyHandlerFactory.h"

namespace masternode {
    class MasternodeConfig {
        public:
            MasternodeConfig(std::string o_host, uint16_t o_port,
                std::string pro_host, HTTPServerOptions&& opts,
                std::vector<HTTPServer::IPConfig> ip_list)
                : origin_host(o_host), origin_port(o_port), protected_host(pro_host),
                    options(opts), IPs(ip_list){};

            // IP or hostname of the origin server (required)
            std::string origin_host;
            // Port of the origin server (required)
            uint16_t origin_port;
            // IP or hostname of the host we're protecting (singular for now) (required)
            std::string protected_host;
            // Proxygen server options
            HTTPServerOptions& options;
            // IPs for the server to locally bind to
            std::vector<HTTPServer::IPConfig> IPs;
    };

    class Masternode {
        public:
            Masternode(MasternodeConfig config) {
                config_ = std::make_shared<MasternodeConfig>(std::move(config));
                server_ = std::make_unique<HTTPServer>(std::move(config_->options));
            };
            ~Masternode() {
                if (server_.get()) {
                    server_->stop();
                }
                run_thread_.join();
            }
            // Blocks until the server successfully starts or fails.
            // Returns true if start() was successful, false if not.
            bool start() {
                bool exception_happened = false;
                server_->bind(config_->IPs);
                boost::barrier barrier{2};
                run_thread_ = std::thread([&]() {
                    server_->start([&]() { barrier_.wait(); },
                                    [&](std::exception_ptr) {
                                        exception_happened = true;
                                        server_.reset();
                                        barrier_.wait();
                                    });
                });
                barrier_.wait();
                return !exception_happened;
            }

            void stop() {
                if (server_.get()) {
                    server_->stop();
                }
            }

        private:
            std::shared_ptr<MasternodeConfig> config_;
            std::unique_ptr<HTTPServer> server_;
            std::thread run_thread_;
            boost::barrier barrier_{2};
    };
}
