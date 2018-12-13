#include <gtest/gtest.h>
#include <glog/logging.h>
#include <boost/thread.hpp>

#include <proxygen/httpserver/HTTPServer.h>

#include <httplib.h>

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

// Wrapper/helper class around an httplib Server object
// so it can run in another thread.
class OriginThread {
 private:
  std::thread t_;
  httplib::Server& server_;

 public:
  explicit OriginThread(httplib::Server& server) : server_(server) {}
  ~OriginThread() {
    // Stop the server and join the thread back in
    // automatically when a unit test finishes.
    server_.stop();
    t_.join();
  }

  void start() {
    t_ = std::thread([&]() {
      server_.listen("0.0.0.0", 8085);
    });
  }
};

TEST (ProxyHandlerFactory, TestPassthroughProxy) {
  // Create an origin server
  auto origin = std::make_unique<httplib::Server>();
  auto origin_thread = std::make_unique<OriginThread>(origin.get()
    ->Get("/", [](const httplib::Request& req, httplib::Response& res) {
        res.set_content("Origin server content", "text/plain");
      }));
  origin_thread->start();

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