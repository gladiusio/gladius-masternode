#include <gtest/gtest.h>
#include <glog/logging.h>
#include <boost/thread.hpp>

#include <folly/FileUtil.h>
#include <folly/experimental/TestUtil.h>

#include <proxygen/lib/utils/TestUtils.h>
#include <proxygen/httpserver/HTTPServer.h>

#define CPPHTTPLIB_OPENSSL_SUPPORT
#include <httplib.h>

#include "NetworkState.h"
#include "Masternode.h"
#include "MasternodeConfig.h"

using namespace folly;
using namespace proxygen;

namespace {
  const std::string testDir = getContainingDirectory(__FILE__).str();
}

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
    masternode::Masternode* master_{nullptr};
  public:
    explicit MasternodeThread(masternode::Masternode* master) : master_(master){}
    ~MasternodeThread() {

      if (master_) {
        master_->stop();
      }
      t_.join();
    }

    bool start() {
      bool throws = false;
      t_ = std::thread([&]() {
        master_->start([&]() { barrier_.wait(); },
                        [&](std::exception_ptr ep) {
                          try {
                            if (ep) {
                              std::rethrow_exception(ep);
                            }
                          } catch(const std::exception& e) {
                            LOG(ERROR) << "Caught exception: " << e.what() << "\n";
                          }
                          throws = true;
                          master_ = nullptr;
                          barrier_.wait();
                        });
      });
      barrier_.wait();
      return !throws;
    }
};

TEST (ContentServing, TestPassthroughProxy) {
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
 
  auto master = std::make_unique<masternode::Masternode>(mc);
  auto master_thread = std::make_unique<MasternodeThread>(master.get());

  EXPECT_TRUE(master_thread->start());

  // Make a request from the client's perspective to the masternode
  httplib::Client client("0.0.0.0", 8080);
  auto res = client.Get("/");
  ASSERT_TRUE(res != nullptr);
  EXPECT_EQ(res->status, 200);
  EXPECT_EQ(res->body, "Origin server content");
}

TEST (ContentServing, TestSSLPassthroughProxy) {
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

  // Enable SSL
  wangle::SSLContextConfig sslCfg;
  sslCfg.isDefault = true;
  sslCfg.setCertificate(testDir + "certs/cert.pem", testDir + "certs/key.pem", "");
  IPs[0].sslConfigs.push_back(sslCfg);

  auto mc = std::make_shared<MasternodeConfig>();
  mc->origin_host = "0.0.0.0";
  mc->origin_port = 8085;
  mc->IPs = IPs;
  mc->cache_directory = "/dev/null";
  mc->options.threads = 1;
  mc->options.idleTimeout = std::chrono::milliseconds(10000);
  mc->options.shutdownOn = {SIGINT, SIGTERM};
  mc->options.enableContentCompression = false;
 
  auto master = std::make_unique<masternode::Masternode>(mc);
  auto master_thread = std::make_unique<MasternodeThread>(master.get());

  ASSERT_TRUE(master_thread->start());

  // Make a request from the client's perspective to the masternode
  httplib::SSLClient client("0.0.0.0", 8080);
  auto res = client.Get("/");
  ASSERT_TRUE(res != nullptr);
  EXPECT_EQ(res->status, 200);
  EXPECT_EQ(res->body, "Origin server content");
}

TEST (NetworkState, TestStateParsing) {
  auto mc = std::make_shared<MasternodeConfig>();
  auto state = std::make_unique<NetworkState>(mc);
  auto sample = R"({"response": {"node_data_map": {"0xdeadbeef": {"content_port": {"data": "8080"}, "ip_address": {"data": "127.0.0.1"}}}}})";
  state->parseStateUpdate(sample);
  EXPECT_EQ(state->getEdgeNodes().size(), 1);
  EXPECT_EQ(state->getEdgeNodes()[0], "127.0.0.1:8080");
}

TEST (NetworkState, TestStatePolling) {
  // Create and start a gateway
  auto gw = std::make_unique<httplib::Server>();
  auto gw_thread = std::make_unique<OriginThread>(gw.get()
    ->Get("/api/p2p/state", [](const httplib::Request& req, httplib::Response& res) {
        res.set_content(R"({"response": {"node_data_map": {"0xdeadbeef": {"content_port": {"data": "8080"}, "ip_address": {"data": "127.0.0.1"}}}}})", "application/json");
      }));
  gw_thread->start();

  auto mc = std::make_shared<MasternodeConfig>();
  mc->gateway_poll_interval = 1;
  mc->gateway_address = "0.0.0.0";
  mc->gateway_port = 8085;
  auto state = std::make_unique<NetworkState>(mc);
  state->beginPollingGateway();
  std::this_thread::sleep_for(std::chrono::seconds(2));
  EXPECT_EQ(state->getEdgeNodes().size(), 1);
  EXPECT_EQ(state->getEdgeNodes()[0], "127.0.0.1:8080");
}

TEST (ContentServing, TestServiceWorkerInjection) {
  // Create and start an origin server
  auto origin = std::make_unique<httplib::Server>();
  auto origin_thread = std::make_unique<OriginThread>(origin.get()
    ->Get("/", [](const httplib::Request& req, httplib::Response& res) {
        res.set_content("<html><head></head><body>Test Body</body></html>", "text/html");
      }));
  origin_thread->start();

  const folly::test::TemporaryFile injectScript;
  auto inject = injectScript.path().string();
  ASSERT_TRUE(folly::writeFile(
    folly::StringPiece("service worker script content"), inject.c_str()));

  // Create and start a masternode
  std::vector<HTTPServer::IPConfig> IPs = {
        {folly::SocketAddress("0.0.0.0", 8080, true),
        HTTPServer::Protocol::HTTP}};

  auto mc = std::make_shared<MasternodeConfig>();
  mc->origin_host = "0.0.0.0";
  mc->origin_port = 8085;
  mc->IPs = IPs;
  mc->service_worker_path = inject;
  mc->cache_directory = "/dev/null";
  mc->options.threads = 1;
  mc->options.idleTimeout = std::chrono::milliseconds(10000);
  mc->options.shutdownOn = {SIGINT, SIGTERM};
  mc->options.enableContentCompression = false;
 
  auto master = std::make_unique<masternode::Masternode>(mc);
  auto master_thread = std::make_unique<MasternodeThread>(master.get());

  EXPECT_TRUE(master_thread->start());

  // Make a request from the client's perspective to the masternode
  // to prime the cache
  httplib::Client client("0.0.0.0", 8080);
  auto res = client.Get("/");
  ASSERT_TRUE(res != nullptr);
  EXPECT_EQ(res->status, 200);
  res = nullptr;
  // Make the same request to serve from cache and inject service worker
  res = client.Get("/");
  ASSERT_TRUE(res != nullptr);
  EXPECT_EQ(res->status, 200);
  EXPECT_NE(res->body.find("<script>"), std::string::npos);
  EXPECT_NE(res->body.find("navigator.serviceWorker.register"), std::string::npos);
}

// todo: add a test that runs the masternode itself polling for p2p state

int main(int argc, char **argv) {
  google::InitGoogleLogging(argv[0]);
  google::InstallFailureSignalHandler();
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}