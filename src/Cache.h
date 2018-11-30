
#include <iostream>

#include <folly/io/IOBuf.h>
#include <folly/ssl/OpenSSLHash.h>
#include <folly/gen/File.h>

#include <wangle/client/persistence/LRUInMemoryCache.h>
#include <wangle/client/persistence/SharedMutexCacheLockGuard.h>

#include <proxygen/lib/utils/URL.h>
#include <proxygen/lib/http/HTTPMessage.h>

class CachedRoute {
    public:
        CachedRoute(std::string url,
            std::unique_ptr<folly::IOBuf> data,
            std::shared_ptr<proxygen::HTTPMessage> headers);
        ~CachedRoute();

        std::string getHash();
        std::string getURL();
        std::unique_ptr<folly::IOBuf> getContent();
        std::shared_ptr<proxygen::HTTPMessage> getHeaders();
    private:
        std::string sha256_;
        std::string url_;
        std::unique_ptr<folly::IOBuf> content_;
        std::shared_ptr<proxygen::HTTPMessage> headers_;
};

class MemoryCache {
    public:
        MemoryCache(size_t capacity) : cache_(capacity) {}

        // Retrieve cached content with the URL as the lookup key
        std::shared_ptr<CachedRoute> getCachedRoute(std::string);

        // Add a new CachedRoute entry to the memory cache
        void addCachedRoute(std::string, std::unique_ptr<folly::IOBuf> chain, std::shared_ptr<proxygen::HTTPMessage> headers);

        size_t size() const;

    private:
        // Shared cache object that all handler threads use
        // to serve cached content from. Thread-safe!
        wangle::LRUInMemoryCache<std::string, std::shared_ptr<CachedRoute>, std::mutex> cache_;
        
};
