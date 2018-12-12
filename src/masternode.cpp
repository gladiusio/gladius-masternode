#include <unistd.h>

#include <folly/Memory.h>
#include <folly/executors/IOThreadPoolExecutor.h>
#include <folly/io/async/EventBaseManager.h>

#include <proxygen/httpserver/HTTPServer.h>
#include <proxygen/httpserver/RequestHandlerFactory.h>

#include "ProxyHandlerFactory.h"

using namespace proxygen;
using namespace masternode;

using folly::HHWheelTimer;


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

  HTTPServerOptions options;
  options.threads = threads;
  options.idleTimeout = std::chrono::milliseconds(60000);
  options.shutdownOn = {SIGINT, SIGTERM};
  options.enableContentCompression = false;
  options.handlerFactories =
      RequestHandlerChain()
          .addThen<ProxyHandlerFactory>()
          .build();

  HTTPServer server(std::move(options));
  server.bind(IPs);

  std::thread t([&]() { server.start(); });

  t.join();

  LOG(INFO) << "Process exiting now\n";

  return 0;
}