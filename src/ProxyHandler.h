#pragma once

#include "Cache.h"
#include "MasternodeConfig.h"
#include "ServiceWorker.h"

#include <folly/Memory.h>
#include <folly/executors/IOThreadPoolExecutor.h>
#include <folly/futures/Future.h>

#include <proxygen/httpserver/RequestHandler.h>
#include <proxygen/lib/http/HTTPConnector.h>
#include <proxygen/lib/http/session/HTTPTransaction.h>


class ProxyHandler : public proxygen::RequestHandler,
                        private proxygen::HTTPConnector::Callback {
    public:
        ProxyHandler(folly::HHWheelTimer *timer,
            std::shared_ptr<ContentCache> cache,
            std::shared_ptr<MasternodeConfig> config, 
            std::shared_ptr<ServiceWorker> sw);

        // RequestHandler methods
        void onRequest(std::unique_ptr<proxygen::HTTPMessage> headers) noexcept override;
        void onBody(std::unique_ptr<folly::IOBuf> body) noexcept override;
        void onUpgrade(proxygen::UpgradeProtocol protocol) noexcept override;
        void onEOM() noexcept override;
        void requestComplete() noexcept override;
        void onError(proxygen::ProxygenError err) noexcept override;
    
        // HTTPConnector::Callback methods
        void connectSuccess(proxygen::HTTPUpstreamSession* session) noexcept override;
        void connectError(const folly::AsyncSocketException& ex) noexcept override;

        // HTTPTransactionHandler delegated methods
        void originSetTransaction(proxygen::HTTPTransaction* txn) noexcept;
        void originDetachTransaction() noexcept;
        void originOnHeadersComplete(std::unique_ptr<proxygen::HTTPMessage> msg) noexcept;
        void originOnBody(std::unique_ptr<folly::IOBuf> chain) noexcept;
        void originOnChunkHeader(size_t length) noexcept;
        void originOnChunkComplete() noexcept;
        void originOnTrailers(std::unique_ptr<proxygen::HTTPHeaders> trailers) noexcept;
        void originOnEOM() noexcept;
        void originOnUpgrade(proxygen::UpgradeProtocol protocol) noexcept;
        void originOnError(const proxygen::HTTPException& error) noexcept;
        void originOnEgressPaused() noexcept;
        void originOnEgressResumed() noexcept;
        void originOnPushedTransaction(proxygen::HTTPTransaction *txn) noexcept;
    private:
        class OriginTransactionHandler : public proxygen::HTTPTransactionHandler {
            public:
                explicit OriginTransactionHandler(ProxyHandler& parent) : parent_(parent) {}
            private:
                void setTransaction(proxygen::HTTPTransaction* txn) noexcept {
                    parent_.originSetTransaction(txn);
                }

                void detachTransaction() noexcept {
                    parent_.originDetachTransaction();
                }

                void onHeadersComplete(std::unique_ptr<proxygen::HTTPMessage> msg) noexcept {
                    parent_.originOnHeadersComplete(std::move(msg));
                }

                void onBody(std::unique_ptr<folly::IOBuf> chain) noexcept {
                    parent_.originOnBody(std::move(chain));
                }

                void onChunkHeader(size_t length) noexcept {
                    parent_.originOnChunkHeader(length);
                }

                void onChunkComplete() noexcept {
                    parent_.originOnChunkComplete();
                }

                void onTrailers(std::unique_ptr<proxygen::HTTPHeaders> trailers) noexcept {
                    parent_.originOnTrailers(std::move(trailers));
                }

                void onEOM() noexcept {
                    parent_.originOnEOM();
                }

                void onUpgrade(proxygen::UpgradeProtocol protocol) noexcept {
                    parent_.originOnUpgrade(protocol);
                }

                void onError(const proxygen::HTTPException& error) noexcept {
                    parent_.originOnError(error);
                }

                void onEgressPaused() noexcept {
                    parent_.originOnEgressPaused();
                }

                void onEgressResumed() noexcept {
                    parent_.originOnEgressResumed();
                }

                void onPushedTransaction(proxygen::HTTPTransaction *txn) noexcept {
                    parent_.originOnPushedTransaction(txn);
                }

                ProxyHandler& parent_;
        };

        // Creates a connection to origin servers we're protecting when we need
        // to fetch content.
        proxygen::HTTPConnector connector_;

        // Handles connection lifecycle events between origin servers and us
        OriginTransactionHandler originHandler_;

        // HTTP transaction used to get content from an origin server
        proxygen::HTTPTransaction* originTxn_{nullptr};

        // Incoming request (headers)
        std::unique_ptr<proxygen::HTTPMessage> request_;

        // Content received from the origin. Used to collect data as it
        // comes in from the origin and later pass in to the cache once
        // all the data is there.
        std::unique_ptr<folly::IOBuf> contentBody_{nullptr};

        // Origin response headers
        std::shared_ptr<proxygen::HTTPMessage> contentHeaders_{nullptr};

        // HTTP content cache
        std::shared_ptr<ContentCache> cache_;

        // Configuration class
        std::shared_ptr<MasternodeConfig> config_;

        // Service worker wrapper
        std::shared_ptr<ServiceWorker> sw_;
}; 

