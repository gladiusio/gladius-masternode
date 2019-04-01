#include "Config.h"

Config::Config() {}

// Creates a Config object based on values provided by
// a TOML configuration file.
Config::Config(const std::string& path) {
    // read config file (can throw, let it bubble up)
    auto toml = cpptoml::parse_file(path);

    // to implement:
    // first, map config file values into struct values
    serverConfig_ = createServerConfig(toml);
    featConfig_ = createFeaturesConfig(toml);
    protDomainsConfig_ = createProtectedDomainsConfig(toml);
    sslConfig_ = createSSLConfig(toml);
}

ServerConfig Config::createServerConfig(std::shared_ptr<cpptoml::table> c) {
    ServerConfig sConfig;
    sConfig.ip = c->get_qualified_as<std::string>("server.ip")
        .value_or("0.0.0.0");
    sConfig.port = c->get_qualified_as<uint16_t>("server.port")
        .value_or(80);
    sConfig.threads = c->get_qualified_as<size_t>("server.threads")
        .value_or(sysconf(_SC_NPROCESSORS_ONLN));
    sConfig.idleTimeoutMs = c->get_qualified_as<uint32_t>("server.idle_timeout_ms")
        .value_or(20000);
    sConfig.cache = createCacheConfig(c);
    return sConfig;
}

CacheConfig Config::createCacheConfig(std::shared_ptr<cpptoml::table> c) {
    CacheConfig cConfig;
    cConfig.maxEntries = c->get_qualified_as<uint32_t>("server.cache.max_entries").value_or(1024);
    cConfig.diskPath = c->get_qualified_as<std::string>("server.cache.disk_path").value_or("");
    return cConfig;
}

FeaturesConfig Config::createFeaturesConfig(
    std::shared_ptr<cpptoml::table> c)
{
    FeaturesConfig fConfig;
    fConfig.p2pConfig = createP2PConfig(c);
    fConfig.sw = createSWConfig(c);
    fConfig.compression = createCompressionConfig(c);
    return fConfig;
}

P2PConfig Config::createP2PConfig(std::shared_ptr<cpptoml::table> c) {
    P2PConfig pConfig;
    pConfig.enabled = c->get_qualified_as<bool>("features.peer-to-peer-cdn.enabled").value_or(false);
    pConfig.gatewayHost = c->get_qualified_as<std::string>("features.peer-to-peer-cdn.gladius_gateway_address").value_or("127.0.0.1");
    pConfig.gatewayPort = c->get_qualified_as<uint16_t>("features.peer-to-peer-cdn.gladius_gateway_port").value_or(3001);
    pConfig.poolDomain = c->get_qualified_as<std::string>("features.peer-to-peer-cdn.pool_domain").value_or("");
    pConfig.cdnSubdomain = c->get_qualified_as<std::string>("features.peer-to-peer-cdn.cdn_subdomain").value_or("");
    pConfig.pollInterval = c->get_qualified_as<uint16_t>("features.peer-to-peer-cdn.poll_interval").value_or(5);
    pConfig.ignoreHeartbeat = c->get_qualified_as<bool>("features.peer-to-peer-cdn.ignore_heartbeat").value_or(false);
    return pConfig;
}

ServiceWorkerConfig Config::createSWConfig(std::shared_ptr<cpptoml::table> c) {
    ServiceWorkerConfig swConfig;
    swConfig.enabled = c->get_qualified_as<bool>("features.service-worker-injection.enabled").value_or(false);
    swConfig.path = c->get_qualified_as<std::string>("features.service-worker-injection.path").value_or("");
    return swConfig;
}

CompressionConfig Config::createCompressionConfig(
    std::shared_ptr<cpptoml::table> c)
{
    CompressionConfig cConfig;
    cConfig.enabled = c->get_qualified_as<bool>("features.compression.enabled")
        .value_or(false);
    cConfig.level = c->get_qualified_as<uint8_t>("features.compression.level")
        .value_or(4);
    cConfig.minSize = c->get_qualified_as<uint64_t>("features.compression.minimum_size")
        .value_or(1024);
    return cConfig;
}

ProtectedDomainsConfig Config::createProtectedDomainsConfig(std::shared_ptr<cpptoml::table> c) {
    ProtectedDomainsConfig pConfig;
    auto domains = c->get_table_array("protected-domain");
    for (const auto& table : *domains) {
        ProtectedDomain pd;
        pd.domain = table->get_as<std::string>("domain_name").value_or("");
        pd.originHost = table->get_as<std::string>("origin_server_host").value_or("");
        pd.originPort = table->get_as<uint16_t>("origin_server_port").value_or(80);
        pConfig.protectedDomains.push_back(pd);
    }
    return pConfig;
}

SSLConfig Config::createSSLConfig(std::shared_ptr<cpptoml::table> c) {
    SSLConfig sConfig;
    auto ssl = c->get_table("ssl");
    sConfig.enabled = ssl->get_as<bool>("enabled").value_or(false);
    sConfig.upgradeInsecure = ssl->get_as<bool>("upgrade_insecure").value_or(false);
    sConfig.port = ssl->get_as<uint16_t>("port").value_or(443);
    auto certs = ssl->get_table_array("certificate");
    for (const auto& table : *certs) {
        SSLCertificateConfig sc;
        sc.certPath = table->get_as<std::string>("cert_path").value_or("");
        sc.keyPath = table->get_as<std::string>("key_path").value_or("");
        sc.isDefault = table->get_as<bool>("is_default").value_or(false);
        sConfig.certs.push_back(sc);
    }
    return sConfig;
}

ServerConfig Config::getServerConfig() const { return serverConfig_; }
FeaturesConfig Config::getFeaturesConfig() const { return featConfig_; }
ProtectedDomainsConfig Config::getProtectedDomainsConfig() const { return protDomainsConfig_; }
SSLConfig Config::getSSLConfig() const { return sslConfig_; }

void Config::setServerConfig(ServerConfig c) { serverConfig_ = c; }
void Config::setFeaturesConfig(FeaturesConfig c) { featConfig_ = c; }
void Config::setProtectedDomainsConfig(ProtectedDomainsConfig c) { protDomainsConfig_ = c; }
void Config::setSSLConfig(SSLConfig c) { sslConfig_ = c; }
