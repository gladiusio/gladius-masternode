#include <gtest/gtest.h>
#include <memory>

#include "EdgeNode.h"

TEST (EdgeNode, TestConstructor) {
    auto node = std::make_shared<EdgeNode>(
        "127.0.0.1", 1234, "0xdeadbeef", 12345678
    );
    EXPECT_EQ(node->getIP(), "127.0.0.1");
    EXPECT_EQ(node->getPort(), 1234);
    EXPECT_EQ(node->getEthAddress(), "0xdeadbeef");
    EXPECT_EQ(node->getHeartbeat(), 12345678);
}

TEST (EdgeNode, TestGetFQDN) {
    auto node = std::make_shared<EdgeNode>(
        "127.0.0.1", 1234, "0xdeadbeef", 12345678
    );
    EXPECT_EQ(node->getFQDN("examplepool.com", "foocdn"), 
        "https://0xdeadbeef.foocdn.examplepool.com:1234");
}
