#pragma once

#include <string.h>

#include <proxygen/httpserver/HTTPServer.h>

class MasternodeConfig {
    public:
        // IP address to bind to locally to serve requests
        std::string ip{""};
        // Port to bind to locally to serve requests
        uint16_t port{80};
        // Internal indicator for whether SSL features should be used
        // (not exposed to user config)
        bool ssl_enabled{false};
        // IP or hostname of the origin server (required)
        std::string origin_host{""};
        // Port of the origin server (required)
        uint16_t origin_port{80};
        // Domain we're protecting (singular for now) (required)
        std::string protected_domain{""};
        // Proxygen server options
        proxygen::HTTPServerOptions options;
        // IPs for the server to locally bind to
        std::vector<proxygen::HTTPServer::IPConfig> IPs;
        // Directory to store cached files
        std::string cache_directory{"/dev/null"};
        // Enable p2p network integration
        bool enableP2P{false};
        // Address of the masternode's Gladius network gateway process
        std::string gateway_address{""};
        // Port of the masternode's Gladius network gateway process
        uint16_t gateway_port{3001};
        // P2P polling interval in seconds
        uint16_t gateway_poll_interval{5};
        // file path to service worker file to serve
        std::string service_worker_path{""};
        // enable/disable service worker injection
        bool enableServiceWorker{true};
        // Enables upgrading HTTP requests to HTTPS via redirects
        bool upgrade_insecure{false};
        // Port to listen to ssl requests
        uint16_t ssl_port{443};
        // Ignore the heartbeat on edge nodes
        bool ignore_heartbeat{false};
        // Domain to use for pool hosts
        std::string pool_domain{""};
        // Subdomain of the pool domain to use for content node hostnames
        std::string cdn_subdomain{"cdn"};
        // Path to the Gladius base directory
        std::string gladius_base{""};
        // GeoIP enabled
        bool geo_ip_enabled{false};
        // Maximum number of routes to cache
        size_t maxRoutesToCache{1024};
};