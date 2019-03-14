#include "Cache.h"
#include <folly/DynamicConverter.h>

CachedRoute::CachedRoute(std::string& url,
    std::unique_ptr<folly::IOBuf> data,
    std::shared_ptr<proxygen::HTTPMessage> headers):
        url_(url), content_(std::move(data)),
        headers_(std::move(headers)) {
    auto out = std::vector<uint8_t>(32);
    folly::ssl::OpenSSLHash::sha256(folly::range(out), *(content_.get()));
    sha256_ = folly::hexlify(out);
}

std::string CachedRoute::getHash() const { return sha256_; }
std::string CachedRoute::getURL() const { return url_; }
std::unique_ptr<folly::IOBuf>
    CachedRoute::getContent() const { return content_->clone(); }
std::shared_ptr<proxygen::HTTPMessage>
    CachedRoute::getHeaders() const { return headers_; }

/////////////////////////////////////////////////////////////////////////

std::shared_ptr<CachedRoute>
    ContentCache::getCachedRoute(std::string url) const {
    auto item = map_.find(url);
    if (item == map_.cend()) {
        // URL is not in the cache
        return nullptr;
    }
    
    return item->second;
}

bool ContentCache::addCachedRoute(std::string url,
    std::unique_ptr<folly::IOBuf> chain,
    std::shared_ptr<proxygen::HTTPMessage> headers) {
    
    // Create a new CachedRoute class
    std::shared_ptr<CachedRoute> newEntry = 
        std::make_shared<CachedRoute>(url, chain->clone(), std::move(headers));
    
    // Insert the CachedRoute class into the cache
    if (!map_.insert(url, newEntry).second) { // blocks for write access
        VLOG(1) << "Could not add route into cache: " << url;
        return false;
    }
    size_t dataSize = newEntry->getContent()->computeChainDataLength();
    VLOG(1) << "Route chain byte size: " << dataSize;
    LOG(INFO) << "Added new cached route: " << url;

    if (this->writeToDisk_) {
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
    auto map = std::make_shared
        <folly::F14FastMap<std::string, std::string>>(this->size());
    for (auto it = map_.cbegin(); it != map_.cend(); ++it) {
        map->insert(std::make_pair(it->first, it->second->getHash()));
    }
    return map;
}

size_t ContentCache::size() const { return map_.size(); }
