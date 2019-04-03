#pragma once

#include <folly/io/IOBuf.h>
#include <folly/gen/File.h>
#include <folly/container/F14Map.h>
#include <folly/concurrency/ConcurrentHashMap.h>

#include <proxygen/lib/http/HTTPMessage.h>

#include "CachedRoute.h"

class ContentCache {
    public:
        const size_t DEFAULT_INITIAL_CACHE_SIZE = 64;
        const size_t DEFAULT_MAX_CACHE_SIZE = 1024;

        ContentCache(const std::vector<std::string>& domains,
            const std::string& dir);

        // Retrieve cached content with the URL as the lookup key
        std::shared_ptr<CachedRoute> getCachedRoute(std::string) const;

        // Add a new CachedRoute entry to the memory cache
        bool addCachedRoute(std::string domain, std::string url,
            std::unique_ptr<folly::IOBuf> chain,
            std::shared_ptr<proxygen::HTTPMessage> headers);

        std::shared_ptr<folly::F14FastMap<std::string, std::string>>
            getAssetHashMap() const;

        size_t size() const;

        std::shared_ptr<folly::ConcurrentHashMap<std::string, std::shared_ptr<CachedRoute>>>
            getDomainCache(const std::string& domain);

    private:
        // thread-safe map of domain:cache-for-domain
        folly::ConcurrentHashMap<std::string, 
            std::shared_ptr<folly::ConcurrentHashMap<std::string, 
            std::shared_ptr<CachedRoute>>>> domainCaches_;
        
        // Root directory for cache file structure
        std::string cache_directory_;

        // Flag to enable writing cached content to disk
        bool writeToDisk_{false};
};
