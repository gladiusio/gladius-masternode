#pragma once

#include <string.h>
#include <stdint.h>

#include <proxygen/httpserver/HTTPServer.h>

class MasternodeConfig {
    public:
        // IP or hostname of the origin server (required)
        std::string origin_host;
        // Port of the origin server (required)
        uint16_t origin_port;
        // Domain we're protecting (singular for now) (required)
        std::string protected_domain;
        // Proxygen server options
        proxygen::HTTPServerOptions options;
        // IPs for the server to locally bind to
        std::vector<proxygen::HTTPServer::IPConfig> IPs;
        // Directory to store cached files
        std::string cache_directory;
        // Address of the masternode's Gladius network gateway process
        std::string gateway_address{""};
        // Port of the masternode's Gladius network gateway process
        uint16_t gateway_port{0};
        // P2P polling interval
        uint16_t gateway_poll_interval{5};
        // file path to service worker file to serve
        std::string service_worker_path;
        // Enables upgrading HTTP requests to HTTPS via redirects
        bool upgrade_insecure;
        // Port to listen to ssl requests
        uint16_t ssl_port;
};