#pragma once

#include <string.h>

#include <proxygen/httpserver/HTTPServer.h>

typedef struct CompressionConfig {
    bool enabled;
    uint8_t level;      
} CompressionConfig;

typedef struct CacheConfig {
    uint32_t maxEntries;
    std::string diskPath;
} CacheConfig;

typedef struct ServerConfig {
    std::string ip;
    uint16_t port;
    CacheConfig cache;
} ServerConfig;

typedef struct P2PConfig {
    bool enabled;
    std::string gatewayHost;
    uint16_t gatewayPort;
    std::string poolDomain;
    std::string cdnSubdomain;
    uint16_t pollInterval;
} P2PConfig;

typedef struct ServiceWorkerConfig {
    bool enabled;
    std::string path;
} ServiceWorkerConfig;

typedef struct FeaturesConfig {
    P2PConfig p2pConfig;
    ServiceWorkerConfig swConfig;
    CompressionConfig compressionConfig;
} FeaturesConfig;

typedef struct ProtectedDomain {
    std::string domain;
    std::string originHost;
    uint16_t originPort;
} ProtectedDomain;

typedef struct ProtectedDomainsConfig {
    std::vector<ProtectedDomain> protectedDomains;
} ProtectedDomainsConfig;

typedef SSLCertificateConfig {
    std::string certPath;
    std::string keyPath;
} SSLCertificateConfig;

typedef struct SSLConfig {
    bool enabled;
    bool upgradeInsecure;
    uint16_t port;
    std::vector<SSLCertificateConfig> certs;
} SSLConfig;

class Config {
    private:
        bool validate_{false};
        ServerConfig serverConfig_{nullptr};
        FeaturesConfig featConfig_{nullptr};
        ProtectedDomainsConfig protDomainsConfig_{nullptr};
        SSLConfig sslConfig_{nullptr};

        ServerConfig createServerConfig(std::shared_ptr<cpptoml::table> c);
        CacheConfig createCacheConfig(std::shared_ptr<cpptoml::table> c);
        FeaturesConfig createFeaturesConfig(std::shared_ptr<cpptoml::table> c);
        P2PConfig createP2PConfig(std::shared_ptr<cpptoml::table> c);
        ServiceWorkerConfig createSWConfig(std::shared_ptr<cpptoml::table> c);
        CompressionConfig createCompressionConfig(
            std::shared_ptr<cpptoml::table> c);
        ProtectedDomainsConfig createProtectedDomainsConfig(std::shared_ptr<cpptoml::table> c);
        SSLConfig createSSLConfig(std::shared_ptr<cpptoml::table> c);
    public:

        // constructor to use if you are going to manually set
        // individual fields in the config
        Config(bool validate = false);
        
        // constructor to use if you wish to populate the config
        // values using a TOML config file
        Config(const std::string& path, bool validate = true);
    
        // todo: move HTTPServerOptions and IPConfig structures/creation
        // to Masternode.cpp. not needed in config 

        // Proxygen server options
        proxygen::HTTPServerOptions options;

        // IPs for the server to locally bind to
        std::vector<proxygen::HTTPServer::IPConfig> IPs;

        // Ignore the heartbeat on edge nodes
        bool ignore_heartbeat{false};
};
