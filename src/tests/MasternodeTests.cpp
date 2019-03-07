#include <gtest/gtest.h>
#include <glog/logging.h>

#include <folly/FileUtil.h>
#include <folly/experimental/TestUtil.h>

#include <proxygen/lib/utils/TestUtils.h>
#include <proxygen/httpserver/HTTPServer.h>

#define CPPHTTPLIB_OPENSSL_SUPPORT
#include <httplib.h>

#include "TestUtils.h"

#include "NetworkState.h"
#include "Masternode.h"
#include "MasternodeConfig.h"

using namespace folly;
using namespace proxygen;

namespace {
  const std::string testDir = getContainingDirectory(__FILE__).str();
}

TEST (Masternode, TestPassthroughProxy) {
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
  EXPECT_EQ(200, res->status);
  EXPECT_EQ("Origin server content", res->body);
}

TEST (Masternode, TestSSLPassthroughProxy) {
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
  EXPECT_EQ(200, res->status);
  EXPECT_EQ("Origin server content", res->body);
}

TEST (Masternode, TestServiceWorkerInjection) {
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

  res = nullptr;
  // now make sure the service worker content is served
  res = client.Get("/gladius-service-worker.js");
  ASSERT_TRUE(res != nullptr);
  EXPECT_EQ(res->status, 200);
  EXPECT_NE(res->body.find("service worker script content"), std::string::npos);
}

TEST (Masternode, TestRedirectHandler) {
  // Create and start a masternode
  std::vector<HTTPServer::IPConfig> IPs = {
        {folly::SocketAddress("0.0.0.0", 8080, true),
        HTTPServer::Protocol::HTTP}};

  // Enable SSL
  HTTPServer::IPConfig sslIP = 
    {folly::SocketAddress("0.0.0.0", 443, true),
    HTTPServer::Protocol::HTTP};
  wangle::SSLContextConfig sslCfg;
  sslCfg.isDefault = true;
  sslCfg.setCertificate(testDir + "certs/cert.pem", testDir + "certs/key.pem", "");
  sslIP.sslConfigs.push_back(sslCfg);
  IPs.push_back(sslIP);

  auto mc = std::make_shared<MasternodeConfig>();
  mc->origin_host = "0.0.0.0";
  mc->origin_port = 8085;
  mc->IPs = IPs;
  mc->cache_directory = "/dev/null";
  mc->options.threads = 1;
  mc->options.idleTimeout = std::chrono::milliseconds(10000);
  mc->options.shutdownOn = {SIGINT, SIGTERM};
  mc->options.enableContentCompression = false;
  mc->upgrade_insecure = true;
  mc->ssl_port = 443;

  auto master = std::make_unique<masternode::Masternode>(mc);
  auto master_thread = std::make_unique<MasternodeThread>(master.get());
  ASSERT_TRUE(master_thread->start());

  // make a request from the client
  httplib::Client client("0.0.0.0", 8080);
  auto res = client.Get("/");
  ASSERT_TRUE(res != nullptr);
  EXPECT_EQ(307, res->status);
  EXPECT_EQ(true, res->has_header("Location"));
  EXPECT_EQ("https://0.0.0.0:443/", res->get_header_value("Location"));
}