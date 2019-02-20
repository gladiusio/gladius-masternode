#pragma once

#include <GeoLite2PP.hpp>

#include "MasternodeConfig.h"

struct Coordinate {
    double latitude;
    double longitude;
};

// This class handles installing the maxmind geoip database
// and provides utility methods wrapping the libmaxmind library
// to access the database. This class will also hold a reference
// to a KD tree of edge nodes to facilitate looking up nodes
// that are nearest to a given client.
class Geo {
    public:
        Geo(std::shared_ptr<MasternodeConfig>);

        // find the geo coordinates of an IP address
        Coordinate lookupCoordinate(std::string);
    private:
        // reference to maxmind geoip database
        GeoLite2PP::DB db_;
};


