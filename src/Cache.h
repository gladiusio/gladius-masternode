
#include <folly/io/IOBuf.h>

#include <wangle/client/persistence/LRUInMemoryCache.h>
#include <wangle/client/persistence/SharedMutexCacheLockGuard.h>

#include <proxygen/lib/utils/URL.h>

class CachedRoute {
    public:
        CachedRoute(proxygen::URL& url, std::unique_ptr<folly::IOBuf> data);
        proxygen::URL getURL();
        std::unique_ptr<folly::IOBuf> getContent();
    private:
        std::string sha256_;
        proxygen::URL url_;
        std::unique_ptr<folly::IOBuf> content_;
};

class MemoryCache {
    public:
        MemoryCache(size_t capacity) : cache_(capacity) {}

        // Retrieve cached content with the URL as the lookup key
        CachedRoute getCachedRoute(const proxygen::URL& url);

        // Add a new CachedRoute entry to the memory cache
        void addCachedRoute(proxygen::URL& url, std::unique_ptr<folly::IOBuf> chain);

    private:
        // Shared cache object that all handler threads use
        // to serve cached content from. Thread-safe!
        wangle::LRUInMemoryCache<proxygen::URL, 
            CachedRoute, wangle::CacheLockGuard<std::mutex>> cache_;
        
};
