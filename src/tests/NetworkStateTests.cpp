#include <gtest/gtest.h>
#include <glog/logging.h>

#include "TestUtils.h"

#include "NetworkState.h"
#include "MasternodeConfig.h"


TEST (NetworkState, TestStateParsing) {
  auto mc = std::make_shared<MasternodeConfig>();
  mc->pool_domain = "example.com";
  auto state = std::make_unique<NetworkState>(mc);
  auto sample = R"({"response": {"node_data_map": {"0xdeadbeef": {"content_port": {"data": "8080"}, "ip_address": {"data": "127.0.0.1"}, "heartbeat": {"data": "999999999"}, "disk_content": {"data": ["yes", "no", "maybe"]}}}}})";
  state->parseStateUpdate(sample, true);
  EXPECT_EQ(state->getEdgeNodes().size(), 1);
  EXPECT_EQ(state->getEdgeNodes()[0], "https://0xdeadbeef.cdn.example.com:8080");
}

TEST (NetworkState, TestEdgeNodeHostnameCreation) {
  auto mc = std::make_shared<MasternodeConfig>();
  mc->pool_domain = "example.com";
  mc->cdn_subdomain = "foobarcdn";
  auto state = std::make_unique<NetworkState>(mc);
  auto actual = state->createEdgeNodeHostname("0xdeadbeeeeeef", "1234");
  EXPECT_EQ(actual, "https://0xdeadbeeeeeef.foobarcdn.example.com:1234");
}

TEST (NetworkState, TestStatePolling) {
  // Create and start a gateway
  auto gw = std::make_unique<httplib::Server>();
  auto gw_thread = std::make_unique<OriginThread>(gw.get()
    ->Get("/api/p2p/state", [](const httplib::Request& req, httplib::Response& res) {
        res.set_content(R"({"response": {"node_data_map": {"0xdeadbeef": {"content_port": {"data": "8080"}, "ip_address": {"data": "127.0.0.1"}, "heartbeat": {"data": "99999999999"}, "disk_content": {"data": ["yes", "no", "maybe"]}}}}})", "application/json");
      }));
  gw_thread->start();

  auto mc = std::make_shared<MasternodeConfig>();
  mc->gateway_poll_interval = 1;
  mc->gateway_address = "0.0.0.0";
  mc->gateway_port = 8085;
  mc->ignore_heartbeat = true;
  mc->pool_domain = "example.com";
  auto state = std::make_unique<NetworkState>(mc);
  state->beginPollingGateway();
  std::this_thread::sleep_for(std::chrono::seconds(2));
  EXPECT_EQ(state->getEdgeNodes().size(), 1);
  EXPECT_EQ(state->getEdgeNodes()[0], "https://0xdeadbeef.cdn.example.com:8080");
}

