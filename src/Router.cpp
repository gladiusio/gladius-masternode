#include <proxygen/lib/utils/URL.h>

#include "Router.h"
#include "DirectHandler.h"
#include "RedirectHandler.h"
#include "ProxyHandler.h"
#include "ServiceWorkerHandler.h"
#include "RejectHandler.h"

Router::Router(std::shared_ptr<Config> config,
    std::shared_ptr<NetworkState> state,
    std::shared_ptr<ContentCache> cache,
    std::shared_ptr<ServiceWorker> sw):
    cache_(cache),
    config_(config),
    state_(state),
    sw_(sw) {
        CHECK(config_) << "Config object was null";
        CHECK(cache_) << "Cache object was null";
        VLOG(1) << "Router created";
    }

void Router::onServerStart(folly::EventBase *evb) noexcept {
    timer_->timer = HHWheelTimer::newTimer(
        evb,
        std::chrono::milliseconds(HHWheelTimer::DEFAULT_TICK_INTERVAL),
        folly::AsyncTimeout::InternalEnum::NORMAL,
        std::chrono::milliseconds(config_->getServerConfig().idleTimeoutMs));
    
    LOG(INFO) << "Server thread now started and listening for requests!";
}

void Router::onServerStop() noexcept {
    timer_->timer.reset();
    LOG(INFO) << "Server thread stopped";
}

// given a proxygen::URL and HTTPMessage, this function will check to see
// if the HTTP request is being made for a domain and port
// that we are configured to serve requests for.
bool Router::requestIsValid(proxygen::URL& url, 
    HTTPMessage* m, 
    std::string& domain)
{
    if (domain.empty()) return false;
    // loop through configured domains and check to see
    // if this request matches any of them

    // TODO: harden this to check that the port of the requested host
    // from the URL or the Host: header matches what this server is configured
    // for. for example, if we're listening on port 80 but someone somehow requests
    // GET ourdomain.com:1234/index or GET /index (Host: ourdomain.com:1234) then
    // we should block that
    for (const auto& pDom: config_->
        getProtectedDomainsConfig().protectedDomains) {
        if (domain == pDom.domain) return true;
    }

    return false;
    // old code for single-domain support
    // if (host == config_->protected_domain) return true;
    // if (config_->ssl_enabled) {
    //     if (host == config_->protected_domain + ":" + std::to_string(config_->ssl_port)) return true;
    // }
    // if (host == config_->protected_domain + ":" + std::to_string(config_->port)) return true;
    // return false;
}

// given a proxygen::URL and HTTPMessage, this function will return the domain
// that the request is made for
std::string Router::determineDomain(proxygen::URL& url, HTTPMessage* m) {
    std::string host = url.getHostAndPort();
    if (host.empty()) {
        host = m->getHeaders().getSingleOrEmpty(HTTP_HEADER_HOST); // could have a port attached to the end
    }

    // strip the port off, if any
    return host.substr(0, host.find(":"));
}

RequestHandler* Router::onRequest(
    RequestHandler *req, HTTPMessage *m) noexcept {
    proxygen::URL url(m->getURL()); // parse the request URL

    std::string domain = determineDomain(url, m);

    if (!requestIsValid(url, m, domain)) {
        // reject this request
        return new RejectHandler(400, "Bad Request");
    }
 
    
    // logic to determine which handler to create based on the request

    // if upgrading insecure requests is enabled and this request is
    // not secure, redirect to the secure port
    if (config_->getSSLConfig().enabled && 
        config_->getSSLConfig().upgradeInsecure &&
        !m->isSecure()) {
        return new RedirectHandler(config_);
    }

    // For speaking directly to the masternode and not an underlying
    // (protected) domain
    if (m->getHeaders().rawExists(DIRECT_HEADER_NAME)) {
        return new DirectHandler(cache_, config_, state_, domain);
    }

    if (config_->getFeaturesConfig().sw.enabled && sw_) {
        // serving the service worker javascript file itself
        if (m->getURL() == "/gladius-service-worker.js") {
            return new ServiceWorkerHandler(config_, sw_);
        }
    }

    // all other requests for proxied content
    return new ProxyHandler(timer_->timer.get(), cache_, config_, sw_);
}
