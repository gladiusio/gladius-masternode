#pragma once

#include <string.h>
#include <stdint.h>

#include <proxygen/httpserver/HTTPServer.h>

#include "ProxyHandlerFactory.h"

namespace masternode {
    class MasternodeConfig {
        public:
            // IP or hostname of the origin server
            std::string origin_host;
            // Port of the origin server
            uint16_t origin_port;
            // IP or hostname of the host we're protecting (singular for now)
            std::string protected_host;
            // Proxygen server options
            HTTPServerOptions options;
    };

    class Masternode {
        public:
            Masternode(MasternodeConfig &);
            
        private:
            MasternodeConfig config_;
    }
}
