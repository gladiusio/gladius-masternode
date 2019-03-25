#include "Geo.h"

Geo::Geo() {

}

Geo::Geo(std::string db_path) {
    db_ = std::make_unique<GeoLite2PP::DB>(db_path); // can throw std::system_error
}

Location Geo::lookupCoordinates(const std::string ip) {
    Location location = { 0.0, 0.0, 0.0, 0.0, 0.0 };

    std::string latitude_str = db_->get_field(
        ip, "en", GeoLite2PP::VCStr{ "location", "latitude" });
    std::string longitude_str = db_->get_field(
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

std::shared_ptr<TreeData> Geo::buildTreeData(const std::vector<std::shared_ptr<EdgeNode>>& nodes) {
    auto td = std::make_shared<TreeData>();
    td->cloud.pts = nodes;
    td->tree = std::make_shared<kd_tree_t>(
        3, td->cloud, KDTreeSingleIndexAdaptorParams(10));
    td->tree->buildIndex();
    return td;
}

std::vector<size_t> Geo::getNearestNodes(Location l, int n) {
    // do a knn search
    std::vector<size_t> ret_indices(n);
    std::vector<double> out_dist_sqr(n);
    nanoflann::KNNResultSet<double> resultSet(n);
    resultSet.init(&ret_indices[0], &out_dist_sqr[0]);
    std::vector<double> query_pt{l.x, l.y, l.z};
    { // critical section
        treeData_.rlock()->get()->tree->findNeighbors(resultSet, 
            &query_pt[0], nanoflann::SearchParams(10));
    }
    return ret_indices;
}

void Geo::setTreeData(std::shared_ptr<TreeData> treeData) {
    { // critical section
        treeData_ = treeData;
    }   
}
