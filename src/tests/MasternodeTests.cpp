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

class MasternodeThread {
  private:
    boost::barrier barrier_{2}; // barrier so we can "wait" for the server to start
    std::thread t_;
    Masternode* master_{nullptr};
  public:
    explicit MasternodeThread(Masternode* master) : master_(master){}
    ~MasternodeThread() {
      LOG(INFO) << "Stopping masternode thread...\n";
      if (master_) {
        master_->stop();
      }
      t_.join();
    }

    bool start() {
      bool throws = false;
      t_ = std::thread([&]() {
        master_->start([&]() { barrier_.wait(); },
                        [&](std::exception_ptr) {
                          throws = true;
                          master_ = nullptr;
                          barrier_.wait();
                        });
      });
      barrier_.wait();
      return !throws;
    }
};

TEST (ProxyHandlerFactory, TestPassthroughProxy) {
  // Create and start an origin server
  auto origin = std::make_unique<httplib::Server>();
  auto origin_thread = std::make_unique<OriginThread>(origin.get()
    ->Get("/", [](const httplib::Request& req, httplib::Response& res) {
        res.set_content("Origin server content", "text/plain");
      }));
  origin_thread->start();

  // Create and start a masternode
  std::vector<HTTPServer::IPConfig> IPs = {
        {folly::SocketAddress("0.0.0.0", 8080, true),
        HTTPServer::Protocol::HTTP}};

  auto mc = std::make_shared<MasternodeConfig>();
  mc->origin_host = "0.0.0.0";
  mc->origin_port = 8085;
  mc->IPs = IPs;
  mc->cache_directory = "/dev/null";
  mc->options.threads = 1;
  mc->options.idleTimeout = std::chrono::milliseconds(10000);
  mc->options.shutdownOn = {SIGINT, SIGTERM};
  mc->options.enableContentCompression = false;
  mc->options.handlerFactories =
      RequestHandlerChain()
          .addThen<ProxyHandlerFactory>(mc)
          .build();
 
  
  auto master = std::make_unique<Masternode>(mc);
  auto master_thread = std::make_unique<MasternodeThread>(master.get());
  LOG(INFO) << "Starting master_thread\n";
  EXPECT_TRUE(master_thread->start());

  // Make a request from the client's perspective to the masternode
  httplib::Client client("0.0.0.0", 8080);
  auto res = client.Get("/");
  ASSERT_TRUE(res != nullptr);
  EXPECT_EQ(res->status, 200);
  EXPECT_EQ(res->body, "Origin server content");
}

int main(int argc, char **argv) {
  google::InitGoogleLogging(argv[0]);
  google::InstallFailureSignalHandler();
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}