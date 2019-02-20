#include "Geo.h"

Geo::Geo(std::shared_ptr<MasternodeConfig> config) {
    std::string path = config->gladius_base + "GeoLite2-City.mmdb";
    db_ = GeoLite2PP::DB(path); // can throw std::system_error
}

Coordinate Geo::lookupCoordinate(const std::string ip) {
    Coordinate location = { 0.0, 0.0 };

    std::string latitude_str = db_.get_field(
        ip, "en", GeoLite2PP::VCStr{ "location", "latitude" });
    std::string longitude_str = db_.get_field(
        ip, "en", GeoLite2PP::VCStr{ "location", "longitude" }
    );
    try {
        location.latitude = std::stod(latitude_str); // can throw
        location.longitude = std::stod(longitude_str); // can throw
    } catch (const std::invalid_argument& ia) {
        LOG(ERROR) << "Invalid argument when converting string to double: " 
            << ia.what();
    } catch (const std::out_of_range& oor) {
        LOG(ERROR) << "Out of range exception when converting string to double: " 
            << oor.what();
    }
    
    return location;
}
