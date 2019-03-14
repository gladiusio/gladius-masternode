#include <proxygen/httpserver/ResponseBuilder.h>
#include <proxygen/lib/utils/URL.h>

#include "RedirectHandler.h"

using namespace proxygen;

RedirectHandler::RedirectHandler(std::shared_ptr<MasternodeConfig> config):
    config_(config){}

void RedirectHandler::onRequest(
    std::unique_ptr<HTTPMessage> headers) noexcept {

    std::string host = headers->getHeaders().rawGet("Host");
    std::string redirect_host{""};
    proxygen::URL url(headers->getURL());
    if (url.hasHost()) {
        redirect_host = url.getHost();
    } else {
        // need to use the Host: header to get the destination
        // host and remove the port, if it's there
        if (!host.empty()) {
            size_t colon = host.find_first_of(":");
            if (colon == std::string::npos) {
                redirect_host = host;
            } else {
                redirect_host = host.substr(0, colon);
            }
        } else {
            ResponseBuilder(downstream_)
                .status(400, "Bad Request")
                .sendWithEOM();
        }
    }

    proxygen::URL redirect_url(
        "https",
        redirect_host,
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
    LOG(INFO) << "RedirectHandler encountered an error: " 
        << getErrorString(err);
    delete this; 
}