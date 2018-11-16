#pragma once

#include <folly/Memory.h>
#include <folly/executors/IOThreadPoolExecutor.h>
#include <folly/executors/CPUThreadPoolExecutor.h>
#include <folly/futures/Future.h>

#include <proxygen/httpserver/RequestHandler.h>

using namespace proxygen;

namespace masternode {
    class ProxyHandler : public RequestHandler {
        public:
            explicit ProxyHandler(folly::CPUThreadPoolExecutor *);

            void onRequest(std::unique_ptr<HTTPMessage> headers) noexcept override;
            void onBody(std::unique_ptr<folly::IOBuf> body) noexcept override;
            void onUpgrade(UpgradeProtocol protocol) noexcept override;
            void onEOM() noexcept override;
            void requestComplete() noexcept override;
            void onError(ProxygenError err) noexcept override;
        
        private:
            std::unique_ptr<folly::IOBuf> body_;
            folly::CPUThreadPoolExecutor *cpuPool_;
    }; 
}
