#include "Cache.h"

CachedRoute::CachedRoute(std::string url, std::unique_ptr<folly::IOBuf> data) {
    url_ = std::move(url);
    std::cout << "Created CachedRoute with url: " << url_ << "\n";
    content_ = std::move(data);
    // TODO: take the sha256 hash of the content_ and assign it to the sha256 member here
}

CachedRoute::~CachedRoute() {
    std::cout << "CachedRoute deleted\n";
}

std::string CachedRoute::getURL() {
    return url_;
}

std::unique_ptr<folly::IOBuf> CachedRoute::getContent() {
    return content_.get()->clone();
}

/////////////////////////////////////////////////////////////////////////

std::shared_ptr<CachedRoute> MemoryCache::getCachedRoute(std::string url) {
    folly::Optional<std::shared_ptr<CachedRoute>> cached = cache_.get(url);
    if (cached) {
        std::cout << "retrieved cached copy\n";
        return cached.value();
    }
    return nullptr;
}

void MemoryCache::addCachedRoute(std::string url,
    std::unique_ptr<folly::IOBuf> chain) {
    std::cout << "adding url to cache: " << url << "\n";

    // Create a new CachedRoute class
    std::shared_ptr<CachedRoute> newEntry = std::make_shared<CachedRoute>(url, chain.get()->clone());
    
    // Insert the CachedRoute class into the cache
    cache_.put(url, newEntry);

    std::cout << "size of cache: " << cache_.size() << "\n";
}

size_t MemoryCache::size() const { return cache_.size(); }
