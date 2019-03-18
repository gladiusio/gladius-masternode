#pragma once

#include <proxygen/httpserver/RequestHandler.h>

class RejectHandler : public proxygen::RequestHandler {
    public:
        RejectHandler(int statusCode, std::string msg):
            statusCode_(statusCode), msg_(msg) {};

        // RequestHandler methods
        void onRequest(std::unique_ptr<proxygen::HTTPMessage> headers) noexcept override;
        void onBody(std::unique_ptr<folly::IOBuf> body) noexcept override;
        void onUpgrade(proxygen::UpgradeProtocol protocol) noexcept override;
        void onEOM() noexcept override;
        void requestComplete() noexcept override;
        void onError(proxygen::ProxygenError err) noexcept override;
    private:
        int statusCode_;
        std::string msg_;
};
