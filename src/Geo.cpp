#include "Geo.h"

Geo::Geo(std::shared_ptr<MasternodeConfig> config) {
    std::string path = config->gladius_base + "GeoLite2-City.mmdb";
    int status = MMDB_open(
        path.c_str(),
        MMDB_MODE_MMAP,
        &mmdb_);

    if (MMDB_SUCCESS != status) {
        LOG(ERROR) << "Could not open geoip database!\n";
        config->geo_ip_enabled = false;
    }
    
}

Geo::~Geo() {
    MMDB_close(&mmdb_);
}
