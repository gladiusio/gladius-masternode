#include <gtest/gtest.h>

#include "Geo.h"

TEST (Geo, TestYes) {

    std::shared_ptr<Geo> g;
    try {
        g = std::make_shared<Geo>();
    } catch (const std::system_error& e) {
        // Geolite2 lib throws if there is no database file
    }

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

    auto berlin = std::make_shared<EdgeNode>(
        "127.0.0.1", 1234, "0xabc", 12345
    );
    Location l3 = {52.520008, 13.404954, 0.0, 0.0, 0.0};
    l3.convertToCartesian();
    berlin->setLocation(l3);
    nodes.push_back(berlin);

    auto london = std::make_shared<EdgeNode>(
        "127.0.0.1", 1234, "0xabc", 12345
    );
    Location l4 = {51.509865, 0.118092, 0.0, 0.0, 0.0};
    l4.convertToCartesian();
    london->setLocation(l4);
    nodes.push_back(london);

    // build a tree with nyc, atl, berlin, and london
    g->buildTree(nodes);

    // make a "request" from DC and expect NYC to be the
    // nearest node
    
}
