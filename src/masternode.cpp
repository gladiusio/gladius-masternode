#include <unistd.h>

#include <proxygen/httpserver/HTTPServer.h>

#include "Masternode.h"

using namespace proxygen;
using namespace masternode;

using folly::HHWheelTimer;


DEFINE_string(ip, "0.0.0.0", "IP/Hostname to bind to");
DEFINE_int32(port, 80, "Port to listen for HTTP requests on");


void Masternode::start() {
  server_->bind(config_->IPs);
  server_->start();
}

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

  HTTPServerOptions options;
  options.threads = threads;
  options.idleTimeout = std::chrono::milliseconds(60000);
  options.shutdownOn = {SIGINT, SIGTERM};
  options.enableContentCompression = false;
  options.handlerFactories =
      RequestHandlerChain()
          .addThen<ProxyHandlerFactory>()
          .build();


  MasternodeConfig mc("159.203.172.79", 80, "blog.gladius.io", std::move(options), std::move(IPs));
  Masternode master(mc);


  //HTTPServer server(std::move(options));
  //server.bind(IPs);

  //std::thread t([&]() { server.start(); });

  std::thread t([&]() { master.start(); });
  t.join();

  LOG(INFO) << "Process exiting now\n";

  return 0;
}