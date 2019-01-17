#include "ServiceWorker.h"

#include <folly/FileUtil.h>

ServiceWorker::ServiceWorker(std::string path) {
    if (!folly::readFile(path.c_str(), payload_)) {
        LOG(ERROR) << "Could not read service worker javascript file: " << path;
    }
}

folly::fbstring ServiceWorker::getPayload() { return payload_; }
folly::fbstring ServiceWorker::getInjectScript() { return inject_script_; }