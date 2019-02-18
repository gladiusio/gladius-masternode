#pragma once

#include <maxminddb.h>

#include "MasternodeConfig.h"

// This class handles installing the maxmind geoip database
// and provides utility methods wrapping the libmaxmind library
// to access the database. This class will also hold a reference
// to a KD tree of edge nodes to facilitate looking up nodes
// that are nearest to a given client.
class Geo {
    public:
        Geo(std::shared_ptr<MasternodeConfig>);
        ~Geo();
    private:
        // reference to maxmind geoip database
        MMDB_s mmdb_;
};
