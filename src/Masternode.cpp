#include <unistd.h>
#define STRIP_FLAG_HELP 1 // removes google gflags help messages in the binary
#include <proxygen/httpserver/HTTPServer.h>

#include "ProxyHandlerFactory.h"

using namespace proxygen;
using namespace folly::ssl;
using namespace masternode;

DEFINE_string(ip, "0.0.0.0", "IP/Hostname to bind to");
DEFINE_int32(port, 80, "Port to listen for HTTP requests on");
DEFINE_string(origin_host, "0.0.0.0", "IP/Hostname of protected origin");
DEFINE_int32(origin_port, 80, "Port to contact the origin server on");
DEFINE_string(protected_host, 
    "localhost", "Hostname of protected host"); // i.e. blog.gladius.io
DEFINE_string(cert_path, "", "Path to SSL certificate");
DEFINE_string(key_path, "", "Path to SSL private key");
DEFINE_string(cache_dir, "", "Path to directory to write cached files to");
DEFINE_string(gateway_address, "http://0.0.0.0:3000", 
    "Address to the masternode's Gladius network gateway");

int main(int argc, char *argv[]) {
    gflags::ParseCommandLineFlags(&argc, &argv, true);
    google::InitGoogleLogging(argv[0]);
    google::InstallFailureSignalHandler();

    size_t threads = sysconf(_SC_NPROCESSORS_ONLN);
    LOG(INFO) << "Configuring server to use " << threads << " I/O threads...\n";

    std::vector<HTTPServer::IPConfig> IPs = {
        {folly::SocketAddress(FLAGS_ip, FLAGS_port, true),
        HTTPServer::Protocol::HTTP}};
    LOG(INFO) << "Binding to " << FLAGS_ip << ":" << FLAGS_port << "\n";

    if (!(FLAGS_cert_path.empty() || FLAGS_key_path.empty())) {
        // Enable SSL
        HTTPServer::IPConfig sslIP = 
            {folly::SocketAddress(FLAGS_ip, 443, true),
            HTTPServer::Protocol::HTTP};
        wangle::SSLContextConfig sslCfg;
        sslCfg.isDefault = true;
        sslCfg.setCertificate(FLAGS_cert_path, FLAGS_key_path, "");
        sslIP.sslConfigs.push_back(sslCfg);
        IPs.push_back(sslIP);
    }
    
    auto config = std::make_shared<MasternodeConfig>();
    config->origin_host = FLAGS_origin_host;
    config->origin_port = FLAGS_origin_port;
    config->protected_host = FLAGS_protected_host;
    config->IPs = IPs;
    config->cache_directory = FLAGS_cache_dir;
    config->options.threads = threads;
    config->options.idleTimeout = std::chrono::milliseconds(60000);
    config->options.shutdownOn = {SIGINT, SIGTERM};
    config->options.enableContentCompression = false;
    config->options.handlerFactories =
        RequestHandlerChain()
            .addThen<ProxyHandlerFactory>(config)
            .build();

    Masternode master(config);

    // Start listening for incoming requests
    std::thread t([&]() { master.start(); });
    t.join();

    LOG(INFO) << "Process exiting now\n";

    return 0;
}