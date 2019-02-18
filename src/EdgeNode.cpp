#include "EdgeNode.h"

EdgeNode::EdgeNode(std::string ip, uint16_t port,
    std::string eth_addr, uint32_t heartbeat,
    std::string fqdn): ip_(ip), port_(port), 
    eth_address_(eth_addr), heartbeat_(heartbeat) {}

std::string EdgeNode::getFQDN(std::string pool_domain,
    std::string cdn_subdomain) {
    return std::string(eth_address_ + "." + cdn_subdomain + "." 
        + pool_domain + ":" + port_);
}