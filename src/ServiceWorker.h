#pragma once

#include <folly/io/IOBuf.h>

class ServiceWorker {
    private:
        // service worker javascript content
        std::string payload_;
        // string for payload to inject into index pages
        std::string inject_script_{
            "navigator.serviceWorker.register('gladius-service-worker.js', {scope: './'});fetch(\"/masternode-cache-list\",{method: \"GET\",mode: \"cors\",headers: {\"gladius-masternode-direct\":\"\",},cache: \"no-store\"})"
        };
    public:
        explicit ServiceWorker(const std::string&);
        std::string getPayload() const;
        std::string getInjectScript() const;
        std::string injectServiceWorker(folly::IOBuf) const;
};
