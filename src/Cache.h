
#include <iostream>

#include <folly/io/IOBuf.h>

#include <wangle/client/persistence/LRUInMemoryCache.h>
#include <wangle/client/persistence/SharedMutexCacheLockGuard.h>

#include <proxygen/lib/utils/URL.h>

class CachedRoute {
    public:
        CachedRoute(std::string, std::unique_ptr<folly::IOBuf> data);
        ~CachedRoute();
        std::string getURL();
        std::unique_ptr<folly::IOBuf> getContent();
    private:
        std::string sha256_;
        std::string url_;
        std::unique_ptr<folly::IOBuf> content_;
};

class MemoryCache {
    public:
        MemoryCache(size_t capacity) : cache_(capacity) {}

        // Retrieve cached content with the URL as the lookup key
        std::shared_ptr<CachedRoute> getCachedRoute(std::string);

        // Add a new CachedRoute entry to the memory cache
        void addCachedRoute(std::string, std::unique_ptr<folly::IOBuf> chain);

        size_t size() const;

    private:
        // Shared cache object that all handler threads use
        // to serve cached content from. Thread-safe!
        wangle::LRUInMemoryCache<std::string, std::shared_ptr<CachedRoute>, std::mutex> cache_;
        
};
