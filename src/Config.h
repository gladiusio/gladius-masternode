#pragma once

#include <string.h>
#include <unistd.h>

#include "cpptoml.h"

typedef struct CompressionConfig {
    bool enabled;
    uint8_t level;
    uint64_t minSize;  
} CompressionConfig;

typedef struct CacheConfig {
    uint32_t maxEntries;
    std::string diskPath;
} CacheConfig;

typedef struct ServerConfig {
    std::string ip;
    uint16_t port;
    CacheConfig cache;
    size_t threads;
    uint32_t idleTimeoutMs;
} ServerConfig;

typedef struct P2PConfig {
    bool enabled;
    std::string gatewayHost;
    uint16_t gatewayPort;
    std::string poolDomain;
    std::string cdnSubdomain;
    uint16_t pollInterval;
    bool ignoreHeartbeat;
} P2PConfig;

typedef struct ServiceWorkerConfig {
    bool enabled;
    std::string path;
} ServiceWorkerConfig;

typedef struct FeaturesConfig {
    P2PConfig p2pConfig;
    ServiceWorkerConfig sw;
    CompressionConfig compression;
} FeaturesConfig;

typedef struct ProtectedDomain {
    std::string domain;
    std::string originHost;
    uint16_t originPort;
} ProtectedDomain;

typedef struct ProtectedDomainsConfig {
    std::vector<ProtectedDomain> protectedDomains;
} ProtectedDomainsConfig;

typedef struct SSLCertificateConfig {
    std::string certPath;
    std::string keyPath;
    bool isDefault;
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
        ServerConfig serverConfig_;
        FeaturesConfig featConfig_;
        ProtectedDomainsConfig protDomainsConfig_;
        SSLConfig sslConfig_;

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
        Config();
        
        // constructor to use if you wish to populate the config
        // values using a TOML config file
        Config(const std::string& path);
    
        ServerConfig getServerConfig() const;
        FeaturesConfig getFeaturesConfig() const;
        ProtectedDomainsConfig getProtectedDomainsConfig() const;
        SSLConfig getSSLConfig() const;

        void setServerConfig(ServerConfig c);
        void setFeaturesConfig(FeaturesConfig c);
        void setProtectedDomainsConfig(ProtectedDomainsConfig c);
        void setSSLConfig(SSLConfig c);

        // Ignore the heartbeat on edge nodes
        bool ignore_heartbeat{false};
};
