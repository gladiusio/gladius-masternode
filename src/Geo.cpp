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

Coordinate Geo::lookupCoordinate(std::string ip) {
    Coordinate location = { 0.0, 0.0 };
    int gai_error, mmdb_error;
    MMDB_lookup_result_s result = 
        MMDB_lookup_string(&mmdb_, ip.c_str(), &gai_error, &mmdb_error);
    if (0 != gai_error) {
        LOG(ERROR) << "Could not lookup address information for IP: " << ip;
        return location;
    }
    if (MMDB_SUCCESS != mmdb_error) {
        LOG(ERROR) << "Error when looking up IP " << ip << 
            "in maxmind database: " << MMDB_strerror(mmdb_error);
        return location;
    }
    if (!result.found_entry) {
        return location;
    }
    MMDB_entry_data_s entry_data_lat, entry_data_long;
    int status = MMDB_get_value(&result.entry, &entry_data_lat,
         "location", "latitude", NULL);
    if (MMDB_SUCCESS != status) {
        return location;
    }
    status = MMDB_get_value(&result.entry, &entry_data_long,
        "location", "longitude", NULL);
    if (MMDB_SUCCESS != status) {
        return location;
    }
    
    if (entry_data_lat.has_data) {
        location.latitude = 
            std::stod(std::string(entry_data_lat.utf8_string,
                entry_data_lat.data_size));
    }
    if (entry_data_long.has_data) {
        location.longitude = 
            std::stod(std::string(entry_data_long.utf8_string,
                entry_data_long.data_size));
    }
    return location;
}
