#pragma once

#include <string.h>

class EdgeNode {
    private:
        std::string ip_;
        uint16_t port_;
        std::string eth_address_;
        uint32_t heartbeat_;

    public:
        EdgeNode(std::string,
            uint16_t,
            std::string,
            uint32_t,
            std::string);

        std::string getIP();
        uint16_t getPort();
        std::string getEthAddress();
        uint32_t getHeartbeat();
        std::string getFQDN(std::string, std::string);
};
