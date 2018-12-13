#include <gtest/gtest.h>
#include <glog/logging.h>
#include <boost/thread.hpp>

#include <proxygen/httpserver/HTTPServer.h>

#include "ProxyHandlerFactory.h"

using namespace folly;
using namespace proxygen;
using namespace masternode;

class ServerThread {
 private:
  boost::barrier barrier_{2};
  std::thread t_;
  HTTPServer* server_{nullptr};

 public:

  explicit ServerThread(HTTPServer* server) : server_(server) {}
  ~ServerThread() {
    if (server_) {
      server_->stop();
    }
    t_.join();
  }

  bool start() {
    bool throws = false;
    t_ = std::thread([&]() {
      server_->start([&]() { barrier_.wait(); },
                     [&](std::exception_ptr /*ex*/) {
                       throws = true;
                       server_ = nullptr;
                       barrier_.wait();
                     });
    });
    barrier_.wait();
    return !throws;
  }
};

TEST (MasternodeTests, Foo) {
  EXPECT_EQ (1, 1);
}

TEST (ProxyHandlerFactory, TestStartsWithServer) {


  std::vector<HTTPServer::IPConfig> IPs = {
        {folly::SocketAddress("0.0.0.0", 8080, true),
        HTTPServer::Protocol::HTTP}};

  HTTPServerOptions options;
  options.threads = 1;
  options.idleTimeout = std::chrono::milliseconds(60000);
  options.shutdownOn = {SIGINT, SIGTERM};
  options.enableContentCompression = false;
  options.handlerFactories =
      RequestHandlerChain()
          .addThen<ProxyHandlerFactory>()
          .build();
  auto server = std::make_unique<HTTPServer>(std::move(options));
  auto st = std::make_unique<ServerThread>(server.get());
  server->bind(IPs);
  EXPECT_TRUE(st->start());
}

int main(int argc, char **argv) {
  google::InitGoogleLogging(argv[0]);
  google::InstallFailureSignalHandler();
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}