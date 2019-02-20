#include "Geo.h"

Geo::Geo(std::shared_ptr<MasternodeConfig> config) {
    std::string path = config->gladius_base + "GeoLite2-City.mmdb";
    db_ = GeoLite2PP::DB(path);
}

Coordinate Geo::lookupCoordinate(std::string ip) {
    Coordinate location = { 0.0, 0.0 };
     return location;
}
