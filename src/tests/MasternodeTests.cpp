#include <gtest/gtest.h>
#include <glog/logging.h>

TEST (MasternodeTests, Foo) {
  EXPECT_EQ (1, 1);
}

int main(int argc, char **argv) {
  google::InitGoogleLogging(argv[0]);
  google::InstallFailureSignalHandler();
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}