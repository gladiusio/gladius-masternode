#include "Geo.h"

Geo::Geo(std::shared_ptr<MasternodeConfig> config) {
    std::string path = config->gladius_base + "GeoLite2-City.mmdb";
    db_ = GeoLite2PP::DB(path); // can throw std::system_error
}

Location Geo::lookupCoordinates(const std::string ip) {
    Location location = { 0.0, 0.0, 0.0, 0.0, 0.0 };

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
    location.convertToCartesian();
    return location;
}


std::shared_ptr<kd_tree_t> Geo::buildTree(const std::vector<std::shared_ptr<EdgeNode>>& nodes) {
    PointCloud cloud;
    cloud.pts = nodes;
    auto t = std::make_shared<kd_tree_t>(
        3, cloud, KDTreeSingleIndexAdaptorParams(10));
    t->buildIndex();
    return t;
}

void Geo::setTree(std::shared_ptr<kd_tree_t> tree) {
    tree_ = tree;
}
