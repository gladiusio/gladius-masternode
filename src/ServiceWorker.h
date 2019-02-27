#pragma once

#include <folly/FBString.h>
#include <folly/io/IOBuf.h>

class ServiceWorker {
    private:
        // service worker javascript content
        folly::fbstring payload_;
        // string for payload to inject into index pages
        folly::fbstring inject_script_{
            "navigator.serviceWorker.register('gladius-service-worker.js', {scope: './'});"
        };
    public:
        explicit ServiceWorker(std::string);
        ~ServiceWorker() {}
        folly::fbstring getPayload();
        folly::fbstring getInjectScript();
        folly::fbstring injectServiceWorker(folly::IOBuf);
};
