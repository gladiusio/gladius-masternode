#include "Cache.h"

CachedRoute::CachedRoute(proxygen::URL& url, std::unique_ptr<folly::IOBuf> data) {
    url_ = std::move(url);
    content_ = std::move(data);
    // TODO: take the sha256 hash of the content_ and assign it to the sha256 member here
}

proxygen::URL CachedRoute::getURL() {
    return url_;
}

std::unique_ptr<folly::IOBuf> CachedRoute::getContent() {
    return content_->clone();
}
