#pragma once

#include <folly/FBString.h>
#include <folly/io/IOBuf.h>

class ServiceWorker {
    private:
        // service worker javascript content
        folly::fbstring payload_;
        // string for payload to inject into index pages
        folly::fbstring inject_script_{
            "navigator.serviceWorker.register('gladius-service-worker.js', {scope: './'});fetch(\"/masternode-cache-list\",{method: \"GET\",mode: \"cors\",headers: {\"gladius-masternode-direct\":\"\",},cache: \"no-store\"})"
        };
    public:
        explicit ServiceWorker(const std::string&);
        folly::fbstring getPayload() const;
        folly::fbstring getInjectScript() const;
        folly::fbstring injectServiceWorker(folly::IOBuf) const;
};
