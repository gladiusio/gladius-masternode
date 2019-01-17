#pragma once

#include <folly/FBString.h>

class ServiceWorker {
    private:
        // service worker javascript content
        folly::fbstring payload_;
        // string for payload to inject into index pages
        folly::fbstring inject_script_;
    public:
        ServiceWorker(std::string);
        ~ServiceWorker() {}
        folly::fbstring getPayload();
        folly::fbstring getInjectScript();
};
