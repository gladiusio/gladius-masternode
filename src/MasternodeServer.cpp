#include <unistd.h>
#define STRIP_FLAG_HELP 1 // removes google gflags help messages in the binary
#include <proxygen/httpserver/HTTPServer.h>

#include "Masternode.h"

using namespace proxygen;
using namespace folly::ssl;
using namespace masternode;

DEFINE_string(ip, "0.0.0.0", "IP/Hostname to bind to");
DEFINE_int32(port, 80, "Port to listen for HTTP requests on");
DEFINE_int32(ssl_port, 443, "Port to listen for HTTPS requests on");
DEFINE_string(origin_host, "0.0.0.0", "IP/Hostname of protected origin");
DEFINE_int32(origin_port, 80, "Port to contact the origin server on");
DEFINE_string(protected_domain, 
    "localhost", "Hostname of protected host"); // i.e. blog.gladius.io
DEFINE_string(cert_path, "", "Path to SSL certificate");
DEFINE_string(key_path, "", "Path to SSL private key");
DEFINE_string(cache_dir, "", "Path to directory to write cached files to");
DEFINE_string(gateway_address, "0.0.0.0", 
    "Address to the masternode's Gladius network gateway");
DEFINE_int32(gateway_port, 3001, 
    "Port of the masternode's Gladius network gateway");
DEFINE_string(sw_path, "", "Path to service worker file to serve");
DEFINE_bool(upgrade_insecure, true, "Redirect HTTP requests to HTTPS port");

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