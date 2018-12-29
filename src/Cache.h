#pragma once

#include <iostream>

#include <folly/io/IOBuf.h>
#include <folly/ssl/OpenSSLHash.h>
#include <folly/gen/File.h>
#include <folly/container/F14Map.h>
#include <folly/concurrency/ConcurrentHashMap.h>

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

class ContentCache {
    public:
        ContentCache(size_t size, std::string dir) : map_(size), cache_directory_(dir) {}

        // Retrieve cached content with the URL as the lookup key
        std::shared_ptr<CachedRoute> getCachedRoute(std::string);

        // Add a new CachedRoute entry to the memory cache
        bool addCachedRoute(std::string, std::unique_ptr<folly::IOBuf> chain,
            std::shared_ptr<proxygen::HTTPMessage> headers);

        std::shared_ptr<folly::F14FastMap<std::string, std::string>> getAssetHashMap();

        size_t size() const;

    private:
        // Shared cache object that all handler threads use
        // to serve cached content from. Thread-safe!
        folly::ConcurrentHashMap<std::string /* url */, std::shared_ptr<CachedRoute>> map_;
        
        // Directory to write cached files to
        std::string cache_directory_;
};
