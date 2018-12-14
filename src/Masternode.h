#pragma once

#include <string.h>
#include <stdint.h>

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
            void start();
        private:
            std::shared_ptr<MasternodeConfig> config_;
            std::unique_ptr<HTTPServer> server_;
    };
}
