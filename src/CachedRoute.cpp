#include <folly/ssl/OpenSSLHash.h>

#include "CachedRoute.h"

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
