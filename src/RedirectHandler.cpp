#include <proxygen/httpserver/ResponseBuilder.h>
#include <proxygen/lib/utils/URL.h>

#include "RedirectHandler.h"

using namespace proxygen;

RedirectHandler::RedirectHandler(std::shared_ptr<MasternodeConfig> config):
    config_(config){}

RedirectHandler::~RedirectHandler() {}

void RedirectHandler::onRequest(std::unique_ptr<HTTPMessage> headers) noexcept {
    LOG(INFO) << "Redirect handler received request for: " << headers->getURL();

    proxygen::URL url(headers->getURL());
    std::string host = headers->getHeaders().rawGet("Host");
    proxygen::URL redirect_url(
        "https",
        host,
        config_->ssl_port,
        url.getPath().substr(1),
        url.getQuery(),
        url.getFragment()
    );
    ResponseBuilder(downstream_)
        .status(307, "Temporary Redirect")
        .header("Location", redirect_url.getUrl())
        .sendWithEOM();
}

void RedirectHandler::onBody(std::unique_ptr<folly::IOBuf> body) noexcept {}
void RedirectHandler::onUpgrade(UpgradeProtocol protocol) noexcept {}
void RedirectHandler::onEOM() noexcept {}
void RedirectHandler::requestComplete() noexcept { delete this; }
void RedirectHandler::onError(proxygen::ProxygenError err) noexcept { 
    LOG(INFO) << "RedirectHandler encountered an error: " << getErrorString(err);
    delete this; 
}