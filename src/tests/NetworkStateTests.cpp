#include <gtest/gtest.h>
#include <glog/logging.h>

#include "TestUtils.h"

#include "NetworkState.h"
#include "Config.h"


TEST (NetworkState, TestStateParsing) {
  auto mc = std::make_shared<Config>();
  mc->pool_domain = "example.com";
  auto state = std::make_unique<NetworkState>(mc);
  auto sample = R"({"response": {"node_data_map": {"0xdeadbeef": {"content_port": {"data": "8080"}, "ip_address": {"data": "127.0.0.1"}, "heartbeat": {"data": "999999999"}, "disk_content": {"data": ["yes", "no", "maybe"]}}}}})";
  state->parseStateUpdate(sample, true);

  EXPECT_EQ(1, state->getEdgeNodes().size());
  EXPECT_EQ("https://0xdeadbeef.cdn.example.com:8080", state->getEdgeNodes()[0]->getFQDN("example.com", "cdn"));
}

TEST (NetworkState, TestEdgeNodeHostnameCreation) {
  auto mc = std::make_shared<Config>();
  mc->pool_domain = "example.com";
  mc->cdn_subdomain = "foobarcdn";
  auto state = std::make_unique<NetworkState>(mc);
  auto actual = state->createEdgeNodeHostname("0xdeadbeeeeeef", "1234");
  EXPECT_EQ("https://0xdeadbeeeeeef.foobarcdn.example.com:1234", actual);
}

TEST (NetworkState, TestStatePolling) {
  // Create and start a gateway
  auto gw = std::make_unique<httplib::Server>();
  auto gw_thread = std::make_unique<OriginThread>(gw.get()
    ->Get("/api/p2p/state", [](const httplib::Request& req, httplib::Response& res) {
        res.set_content(R"({"response": {"node_data_map": {"0xdeadbeef": {"content_port": {"data": "8080"}, "ip_address": {"data": "127.0.0.1"}, "heartbeat": {"data": "99999999999"}, "disk_content": {"data": ["yes", "no", "maybe"]}}}}})", "application/json");
      }));
  gw_thread->start();

  auto mc = std::make_shared<Config>();
  mc->gateway_poll_interval = 1;
  mc->gateway_address = "0.0.0.0";
  mc->gateway_port = 8085;
  mc->ignore_heartbeat = true;
  mc->pool_domain = "example.com";
  mc->geo_ip_enabled = false;
  auto state = std::make_unique<NetworkState>(mc);
  state->beginPollingGateway();
  std::this_thread::sleep_for(std::chrono::milliseconds(300));

  EXPECT_EQ(1, state->getEdgeNodes().size());
  EXPECT_EQ("https://0xdeadbeef.cdn.example.com:8080", state->getEdgeNodes()[0]->getFQDN("example.com", "cdn"));
}

TEST (NetworkState, TestGetNearestNodes) {
  std::unique_ptr<Geo> g = std::make_unique<Geo>();

  std::vector<std::shared_ptr<EdgeNode>> nodes;

  auto atl = std::make_shared<EdgeNode>(
      "127.1.1.1", 1234, "0xabc", 12345
  );
  Location l = {33.753746, -84.386330, 0.0, 0.0, 0.0};
  l.convertToCartesian();
  atl->setLocation(l);
  nodes.push_back(atl);

  auto berlin = std::make_shared<EdgeNode>(
      "127.2.2.2", 1234, "0xabc", 12345
  );
  Location l2 = {52.520008, 13.404954, 0.0, 0.0, 0.0};
  l2.convertToCartesian();
  berlin->setLocation(l2);
  nodes.push_back(berlin);

  auto nyc = std::make_shared<EdgeNode>(
      "127.3.3.3", 1234, "0xabc", 12345
  );
  Location l3 = {40.730610, -73.935242, 0.0, 0.0, 0.0};
  l3.convertToCartesian();
  nyc->setLocation(l3);
  nodes.push_back(nyc);

  auto london = std::make_shared<EdgeNode>(
      "127.4.4.4", 1234, "0xabc", 12345
  );
  Location l4 = {51.509865, 0.118092, 0.0, 0.0, 0.0};
  l4.convertToCartesian();
  london->setLocation(l4);
  nodes.push_back(london);

  auto sf = std::make_shared<EdgeNode>(
      "127.5.5.5", 1234, "0xabc", 12345
  );
  Location l5 = {37.773972, -122.431297, 0.0, 0.0, 0.0};
  l5.convertToCartesian();
  sf->setLocation(l5);
  nodes.push_back(sf);

  auto reno = std::make_shared<EdgeNode>(
      "127.6.6.6", 1234, "0xabc", 12345
  );
  Location l6 = {39.530895, -119.814972, 0.0, 0.0, 0.0};
  l6.convertToCartesian();
  reno->setLocation(l6);
  nodes.push_back(reno);

  // build a tree with nyc, atl, berlin, and london
  auto treeData = g->buildTreeData(nodes);
  g->setTreeData(treeData);

  auto mc = std::make_shared<MasternodeConfig>();
  mc->pool_domain = "example.com";
  auto state = std::make_unique<NetworkState>(mc, std::move(g));
  state->setEdgeNodes(nodes);

  // make a "request" from DC and expect NYC to be the
  // nearest node and ATL the second nearest
  Location dc = {	38.889931, -77.009003, 0.0, 0.0, 0.0};
  dc.convertToCartesian();
  auto nearest_nodes = state->getNearestEdgeNodes(dc, 2);
  EXPECT_EQ(2, nearest_nodes.size());
  EXPECT_EQ("127.3.3.3", nearest_nodes.at(0)->getIP());
  EXPECT_EQ("127.1.1.1", nearest_nodes.at(1)->getIP());
}
