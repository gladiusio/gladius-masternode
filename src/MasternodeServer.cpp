#include <unistd.h>
#define STRIP_FLAG_HELP 1 // removes google gflags help messages in the binary
#include <proxygen/httpserver/HTTPServer.h>

#include "Masternode.h"

using namespace proxygen;
using namespace folly::ssl;
using namespace masternode;

DEFINE_string(ip, "0.0.0.0", "The IP address/hostname to listen for requests on");
DEFINE_int32(port, 80, "The port to listen for HTTP requests on");
DEFINE_int32(ssl_port, 443, "The port to listen for HTTPS requests on");
DEFINE_string(origin_host, "0.0.0.0", "The IP/Hostname of the origin server to proxy for");
DEFINE_int32(origin_port, 80, "The port of the origin server to connect to");
DEFINE_string(protected_domain, 
    "localhost", "The domain name we are protecting"); // i.e. www.example.com
DEFINE_string(cert_path, "", "File path to SSL certificate");
DEFINE_string(key_path, "", "File path to the private key for the SSL cert");
DEFINE_string(cache_dir, "", "Path to directory to write cached content to");
DEFINE_string(gateway_address, "0.0.0.0", 
    "IP/Hostname of Gladius network gateway process");
DEFINE_int32(gateway_port, 3001, 
    "Port to reach the Gladius network gateway process on");
DEFINE_string(sw_path, "", "File path of service worker javascript file to inject");
DEFINE_bool(upgrade_insecure, true, "Set to true to redirect HTTP requests to the HTTPS port");
DEFINE_string(pool_domain, "", "Domain to use for pool hosts"); // i.e. examplepool.com
DEFINE_string(cdn_subdomain, "cdn", "Subdomain of the pool domain to use for content node hostnames");
DEFINE_string(gladius_base, "", "Path to the Gladius base directory");
DEFINE_bool(geo_ip_enabled, false, "Set to true to enable geoip request routing");

// debug use only
DEFINE_bool(ignore_heartbeat, false, "Set to true to disable heartbeat checking for edge nodes");

int main(int argc, char *argv[]) {
    gflags::ParseCommandLineFlags(&argc, &argv, true);
    google::InitGoogleLogging(argv[0]);
    google::InstallFailureSignalHandler();

    auto config = std::make_shared<MasternodeConfig>();

    size_t threads = sysconf(_SC_NPROCESSORS_ONLN);
    LOG(INFO) << "Configuring server to use " << threads << " I/O threads...\n";

    std::vector<HTTPServer::IPConfig> IPs = {
        {folly::SocketAddress(FLAGS_ip, FLAGS_port, true),
        HTTPServer::Protocol::HTTP}};
    LOG(INFO) << "Binding to " << FLAGS_ip << ":" << FLAGS_port << "\n";

    if (!(FLAGS_cert_path.empty() || FLAGS_key_path.empty())) {
        // Enable SSL
        config->ssl_port = FLAGS_ssl_port;
        HTTPServer::IPConfig sslIP = 
            {folly::SocketAddress(FLAGS_ip, FLAGS_ssl_port, true),
            HTTPServer::Protocol::HTTP};
        wangle::SSLContextConfig sslCfg;
        sslCfg.isDefault = true;
        sslCfg.clientVerification = folly::SSLContext::SSLVerifyPeerEnum::NO_VERIFY;
        sslCfg.setCertificate(FLAGS_cert_path, FLAGS_key_path, "");
        sslIP.sslConfigs.push_back(sslCfg);
        IPs.push_back(sslIP);
        LOG(INFO) << "Binding to " << FLAGS_ip << ":" << FLAGS_ssl_port << " for SSL requests\n";
        if (FLAGS_upgrade_insecure) {
            config->upgrade_insecure = true;
            LOG(INFO) << "Configured to upgrade requests from HTTP --> HTTPS\n";
        }
    }
    
    config->origin_host = FLAGS_origin_host;
    config->origin_port = FLAGS_origin_port;
    config->protected_domain = FLAGS_protected_domain;
    config->gateway_address = FLAGS_gateway_address;
    config->gateway_port = FLAGS_gateway_port;
    config->service_worker_path = FLAGS_sw_path;
    config->IPs = IPs;
    config->cache_directory = FLAGS_cache_dir;
    config->ignore_heartbeat = FLAGS_ignore_heartbeat;
    config->pool_domain = FLAGS_pool_domain;
    config->cdn_subdomain = FLAGS_cdn_subdomain;
    config->gladius_base = FLAGS_gladius_base;
    config->geo_ip_enabled = FLAGS_geo_ip_enabled;
    config->options.threads = threads;
    config->options.idleTimeout = std::chrono::milliseconds(60000);
    config->options.shutdownOn = {SIGINT, SIGTERM};
    config->options.enableContentCompression = false;

    Masternode master(config);

    // Start listening for incoming requests
    std::thread t([&]() { master.start(); });
    t.join();

    LOG(INFO) << "Process exiting now\n";

    return 0;
}