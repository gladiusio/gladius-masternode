#include "EdgeNode.h"

EdgeNode::EdgeNode(std::string ip, uint16_t port,
    std::string eth_addr, uint32_t heartbeat): ip_(ip), port_(port), 
    eth_address_(eth_addr), heartbeat_(heartbeat) {}

std::string EdgeNode::getIP() { return ip_; }
uint16_t EdgeNode::getPort() { return port_; }
std::string EdgeNode::getEthAddress() { return eth_address_; }
uint32_t EdgeNode::getHeartbeat() { return heartbeat_; }

std::string EdgeNode::getFQDN(std::string pool_domain,
    std::string cdn_subdomain) {
    return std::string(eth_address_ + "." + cdn_subdomain + "." 
        + pool_domain + ":" + std::to_string(port_));
}