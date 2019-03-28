#include <gtest/gtest.h>
#include <glog/logging.h>

#include "Config.h"

TEST (Config, TestConfigSetIP) {
  auto c = std::make_shared<Config>(true);
  EXPECT_NO_THROW(c->setIP("0.0.0.0"));
  EXPECT_EQ("0.0.0.0", c->ip);
  EXPECT_ANY_THROW(c->setIP("foobar"));
}
