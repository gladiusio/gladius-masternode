#include "Cache.h"
#include <folly/DynamicConverter.h>

CachedRoute::CachedRoute(std::string url,
    std::unique_ptr<folly::IOBuf> data,
    std::shared_ptr<proxygen::HTTPMessage> headers) {
    url_ = std::move(url);
    content_ = std::move(data);
    headers_ = std::move(headers);
    auto out = std::vector<uint8_t>(32);
    folly::ssl::OpenSSLHash::sha256(folly::range(out), *(content_.get()));
    sha256_ = folly::hexlify(out);
    LOG(INFO) << "Created a CachedRoute object\n";
}

CachedRoute::~CachedRoute() {
    LOG(INFO) << "Destroyed a CachedRoute object\n";
}

std::string CachedRoute::getHash() { return sha256_; }
std::string CachedRoute::getURL() { return url_; }
std::unique_ptr<folly::IOBuf> CachedRoute::getContent() { return content_->clone(); }
std::shared_ptr<proxygen::HTTPMessage> CachedRoute::getHeaders() { return headers_; }

/////////////////////////////////////////////////////////////////////////

std::shared_ptr<CachedRoute> ContentCache::getCachedRoute(std::string url) {
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
    if (!map_.insert(url, newEntry).second) {
        LOG(INFO) << "Could not add route into cache: " << url << "\n";
        return false;
    }
    LOG(INFO) << "Route byte size: " << newEntry->getContent()->length() << "\n";
    LOG(INFO) << "Route chain byte size: " << newEntry->getContent()->computeChainDataLength() << "\n";
    LOG(INFO) << "Added new cached route: " << url << "\n";

    // write bytes to file
    // todo: use thread pool to do this off of the event IO threads
    folly::File f(cache_directory_ + newEntry->getHash(),
        O_WRONLY | O_CREAT | O_TRUNC, 0666);
    folly::gen::from(*newEntry->getContent()) | 
        folly::gen::toFile(folly::File(f.fd()), newEntry->getContent()->computeChainDataLength());
    return true;
}

std::shared_ptr<folly::F14FastMap<std::string, std::string>> ContentCache::getAssetHashMap() {
    auto map = std::make_shared<folly::F14FastMap<std::string, std::string>>(this->size());
    for (auto it = map_.cbegin(); it != map_.cend(); ++it) {
        map->insert(std::make_pair(it->first, it->second->getHash()));
    }
    return map;
}

size_t ContentCache::size() const { return map_.size(); }
