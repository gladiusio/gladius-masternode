#include "ContentCache.h"

ContentCache::ContentCache(const std::vector<std::string>& domains,
    const std::string& dir) : 
    domainCaches_(domains.size(), domains.size()),
    cache_directory_(dir),
    writeToDisk_(!dir.empty())
{
    for (const auto& domain : domains) {
        // create a map for each domain
        auto map = std::make_shared<folly::ConcurrentHashMap<
            std::string, std::shared_ptr<CachedRoute>>>(
                DEFAULT_INITIAL_CACHE_SIZE,
                DEFAULT_MAX_CACHE_SIZE
            );
        domainCaches_.insert(domain, map);
    }
}

std::shared_ptr<CachedRoute>
    ContentCache::getCachedRoute(std::string url) const {
    // auto item = map_.find(url);
    // if (item == map_.cend()) {
    //     // URL is not in the cache
    //     return nullptr;
    // }
    
    // return item->second;
    return nullptr;
}

// returns the LRUCache for the specified domain
std::shared_ptr<folly::ConcurrentHashMap<std::string, std::shared_ptr<CachedRoute>>>
    ContentCache::getDomainCache(const std::string& domain) 
{
     // find the cache for this domain
    auto map = domainCaches_.find(domain);
    if (map == domainCaches_.cend()) {
        // this domain does not have a cache
        return nullptr;
    }
    return map->second;
}

bool ContentCache::addCachedRoute(std::string domain, std::string path,
    std::unique_ptr<folly::IOBuf> chain,
    std::shared_ptr<proxygen::HTTPMessage> headers) {
    
    auto dCache = getDomainCache(domain);
    if (dCache == nullptr) { return false; }

    // Create a new CachedRoute class
    std::shared_ptr<CachedRoute> newEntry = 
        std::make_shared<CachedRoute>(path, chain->cloneCoalesced(), std::move(headers));
    
    // Insert the CachedRoute class into the cache
    if (!dCache->insert(path, newEntry).second) { // blocks for write access
        VLOG(1) << "Could not add route into cache: " << path;
        return false;
    }
    
    LOG(INFO) << "Added new cached route: " << path;

    if (this->writeToDisk_) {
        size_t dataSize = newEntry->getContent()->computeChainDataLength();
        // write bytes to file
        // todo: use thread pool to do this off of the event IO threads
        folly::File f(cache_directory_ + newEntry->getHash(),
            O_WRONLY | O_CREAT | O_TRUNC, 0666);
        folly::gen::from(*newEntry->getContent()) | 
            folly::gen::toFile(f.dup(), dataSize);
    }
    
    return true;
}

std::shared_ptr<folly::F14FastMap<std::string, std::string>> 
    ContentCache::getAssetHashMap() const {
    // auto map = std::make_shared
    //     <folly::F14FastMap<std::string, std::string>>(this->size());
    // for (auto it = map_.cbegin(); it != map_.cend(); ++it) {
    //     map->insert(std::make_pair(it->first, it->second->getHash()));
    // }
    // return map;
    return nullptr;
}

size_t ContentCache::size() const { 
    // return lrus_.size(); 
    return 0;
}
