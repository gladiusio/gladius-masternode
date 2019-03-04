#include <gtest/gtest.h>

#include "Geo.h"

TEST (Geo, TestGetNearestNodes) {

    Geo g;

    std::vector<std::shared_ptr<EdgeNode>> nodes;

    auto atl = std::make_shared<EdgeNode>(
        "127.0.0.1", 1234, "0xabc", 12345
    );
    Location l = {33.753746, -84.386330, 0.0, 0.0, 0.0};
    l.convertToCartesian();
    atl->setLocation(l);
    nodes.push_back(atl);

    auto berlin = std::make_shared<EdgeNode>(
        "127.0.0.1", 1234, "0xabc", 12345
    );
    Location l2 = {52.520008, 13.404954, 0.0, 0.0, 0.0};
    l2.convertToCartesian();
    berlin->setLocation(l2);
    nodes.push_back(berlin);

    auto nyc = std::make_shared<EdgeNode>(
        "127.0.0.1", 1234, "0xabc", 12345
    );
    Location l3 = {40.730610, -73.935242, 0.0, 0.0, 0.0};
    l3.convertToCartesian();
    nyc->setLocation(l3);
    nodes.push_back(nyc);

    auto london = std::make_shared<EdgeNode>(
        "127.0.0.1", 1234, "0xabc", 12345
    );
    Location l4 = {51.509865, 0.118092, 0.0, 0.0, 0.0};
    l4.convertToCartesian();
    london->setLocation(l4);
    nodes.push_back(london);

    auto sf = std::make_shared<EdgeNode>(
        "127.0.0.1", 1234, "0xabc", 12345
    );
    Location l5 = {37.773972, -122.431297, 0.0, 0.0, 0.0};
    l5.convertToCartesian();
    sf->setLocation(l5);
    nodes.push_back(sf);

    auto reno = std::make_shared<EdgeNode>(
        "127.0.0.1", 1234, "0xabc", 12345
    );
    Location l6 = {39.530895, -119.814972, 0.0, 0.0, 0.0};
    l6.convertToCartesian();
    reno->setLocation(l6);
    nodes.push_back(reno);

    // build a tree with nyc, atl, berlin, and london
    auto tree = g.buildTree(nodes);
    g.setTree(tree);
    // make a "request" from DC and expect NYC to be the
    // nearest node and ATL the second nearest
    Location dc = {	38.889931, -77.009003, 0.0, 0.0, 0.0};
    dc.convertToCartesian();
    std::vector<size_t> node_indices = g.getNearestNodes(dc, 2);
    EXPECT_EQ(2, node_indices.size());
    EXPECT_EQ(2, node_indices.at(0));
    EXPECT_EQ(0, node_indices.at(1));

    // make a "request" from San Jose and expect SF to be the
    // nearest node and Reno the second nearest
    Location sj = {37.279518, -121.867905, 0.0, 0.0, 0.0};
    sj.convertToCartesian();
    node_indices.clear();
    node_indices = g.getNearestNodes(sj, 2);
    EXPECT_EQ(2, node_indices.size());
    EXPECT_EQ(4, node_indices.at(0));
    EXPECT_EQ(5, node_indices.at(1));
}

TEST (Geo, TestBuildTree) {
    Geo g;

    std::vector<std::shared_ptr<EdgeNode>> nodes;
    auto nyc = std::make_shared<EdgeNode>(
        "127.0.0.1", 1234, "0xabc", 12345
    );
    Location l = {40.730610, -73.935242, 0.0, 0.0, 0.0};
    l.convertToCartesian();
    nyc->setLocation(l);
    nodes.push_back(nyc);

    auto atl = std::make_shared<EdgeNode>(
        "127.0.0.1", 1234, "0xabc", 12345
    );
    Location l2 = {33.753746, -84.386330, 0.0, 0.0, 0.0};
    l2.convertToCartesian();
    atl->setLocation(l2);
    nodes.push_back(atl);

    auto tree = g.buildTree(nodes);
    EXPECT_NE(nullptr, tree);
    EXPECT_EQ(2, tree->m_size);
}
