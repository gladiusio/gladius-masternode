#include <unistd.h>

#include <proxygen/httpserver/HTTPServer.h>

#include "ProxyHandlerFactory.h"

using namespace proxygen;
using namespace masternode;

DEFINE_string(ip, "0.0.0.0", "IP/Hostname to bind to");
DEFINE_int32(port, 80, "Port to listen for HTTP requests on");


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

  auto config = std::make_shared<MasternodeConfig>();
  config->options.threads = threads;
  config->options.idleTimeout = std::chrono::milliseconds(60000);
  config->options.shutdownOn = {SIGINT, SIGTERM};
  config->options.enableContentCompression = false;
  config->options.handlerFactories =
      RequestHandlerChain()
          .addThen<ProxyHandlerFactory>(config)
          .build();
  config->origin_host = "159.203.172.79";
  config->origin_port = 80;
  config->protected_host = "blog.gladius.io";
  config->IPs = IPs;
  config->cache_directory = "/home/.gladius/content/blog.gladius.io";
  
  Masternode master(config);

  std::thread t([&]() { master.start(); });
  t.join();

  LOG(INFO) << "Process exiting now\n";

  return 0;
}