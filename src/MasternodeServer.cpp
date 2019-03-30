#include <unistd.h>
#define STRIP_FLAG_HELP 1 // removes google gflags help messages in the binary
#include <proxygen/httpserver/HTTPServer.h>

#include "Masternode.h"

using namespace proxygen;
using namespace folly::ssl;
using namespace masternode;

DEFINE_string(config, "", "Path to the config file to use");

// debug use only
DEFINE_bool(ignore_heartbeat, false, "Set to true to disable heartbeat checking for edge nodes");

int main(int argc, char *argv[]) {
    gflags::ParseCommandLineFlags(&argc, &argv, true);
    google::InitGoogleLogging(argv[0]);
    google::InstallFailureSignalHandler();

    // read the config file
    std::shared_ptr<Config> config;
    try {
        config = std::make_shared<Config>(FLAGS_config);
    } catch (const std::exception& e) {
        LOG(ERROR) << "Could not parse config file.\n" << e.what();
    }

    Masternode master(config);

    // Start listening for incoming requests
    std::thread t([&]() { master.start(); });
    t.join();

    LOG(INFO) << "Process exiting now";

    return 0;
}