#pragma once
#include <folly/io/IOBuf.h>
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
