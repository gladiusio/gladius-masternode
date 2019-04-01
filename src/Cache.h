#pragma once

#include <folly/io/IOBuf.h>
#include <folly/gen/File.h>
#include <folly/container/F14Map.h>
#include <folly/concurrency/ConcurrentHashMap.h>

#include <proxygen/lib/http/HTTPMessage.h>

class CachedRoute {
    public:
        CachedRoute(std::string& url,
            std::unique_ptr<folly::IOBuf> data,
            std::shared_ptr<proxygen::HTTPMessage> headers);

        std::string getHash() const;
        std::string getURL() const;
        std::unique_ptr<folly::IOBuf> getContent() const;
        std::shared_ptr<proxygen::HTTPMessage> getHeaders() const;
    private:
        std::string sha256_;
        std::string url_;
        std::unique_ptr<folly::IOBuf> content_;
        std::shared_ptr<proxygen::HTTPMessage> headers_;
};

// create one of these for each domain registered.
// each instance gets its own subdirectory in the cache directory.
// this class handles storing cached data in memory and disk.
// will handle keeping the memory footprint under a configured threshold
// by evicting items with an LRU policy. will do the same for the disk
// cache layer. if the memory usage is below the threshold and there are
// available cache entries present on disk but not currently in memory,
// this class will load them into memory.
class LRUCache {
    private:
        folly::Synchronized<EvictingCacheMap<std::string,
            std::shared_ptr<CachedRoute>>> map_;
        size_t memSize_;
        size_t diskSize_;
        std::string cache_dir_;
    
    public:
        void add(); // adds to memory cache and writes to disk
        void remove(); // removes from memory cache and disk (useful for purge)
        std::shared_ptr<CachedRoute> get(const std::string& path); // retrieves a cached route by key (path)
};

class ContentCache {
    public:
        const size_t DEFAULT_INITIAL_CACHE_SIZE = 64;
        const size_t DEFAULT_MAX_CACHE_SIZE = 1024;

        ContentCache(size_t maxSize, std::string& dir, bool writeToDisk) : 
            map_(DEFAULT_INITIAL_CACHE_SIZE, maxSize),
            cache_directory_(dir),
            writeToDisk_(writeToDisk) {}

        // Retrieve cached content with the URL as the lookup key
        std::shared_ptr<CachedRoute> getCachedRoute(std::string) const;

        // Add a new CachedRoute entry to the memory cache
        bool addCachedRoute(std::string url,
            std::unique_ptr<folly::IOBuf> chain,
            std::shared_ptr<proxygen::HTTPMessage> headers);

        std::shared_ptr<folly::F14FastMap<std::string, std::string>>
            getAssetHashMap() const;

        size_t size() const;

    private:
        // Shared cache object that all handler threads use
        // to serve cached content from. Thread-safe!
        // Reads are wait-free, writes are locking.
        folly::ConcurrentHashMap<std::string /* url */, 
            std::shared_ptr<CachedRoute>> map_;

        folly::ConcurrentHashMap<std::string, LRUCache> lrus_;
        
        // Directory to write cached files to
        std::string cache_directory_;

        // Flag to enable writing cached content to disk
        bool writeToDisk_{false};
};
