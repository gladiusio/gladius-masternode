#include <gtest/gtest.h>
#include <glog/logging.h>

#include "Config.h"

TEST (Config, TestConfigSetIP) {
  auto c = std::make_shared<Config>(true);
  EXPECT_NO_THROW(c->setIP("0.0.0.0"));
  EXPECT_EQ("0.0.0.0", c->ip);
  // EXPECT_ANY_THROW(c->setIP("foobar"));
}

TEST (Config, TestConfigSetPort) {
  auto c = std::make_shared<Config>(true);
  EXPECT_NO_THROW(c->setPort(80));
  EXPECT_EQ(80, c->port);
  EXPECT_ANY_THROW(c->setPort(-123));
  EXPECT_ANY_THROW(c->setPort(88888));
}
