#include "Geo.h"

// Create a Geo object without a maxmind database
// (used for testing currently)
Geo::Geo() {

}

// Create a Geo object using a maxmind database
// located at the given path 'db_path'
// This constructor can throw a std::system_error
// if there is an issue loading the database file.
Geo::Geo(std::string db_path) {
    db_ = std::make_unique<GeoLite2PP::DB>(db_path); // can throw std::system_error
}

// Given an IP address string, this method will check the
// maxmind database for its longitude and latitude and return
// a Location object populated with those values.
// If a given IP address is not found in the database or there
// are other issues with the lookup, the returned Location
// object will have default coordinates of 0.
Location Geo::lookupCoordinates(const std::string ip) {
    VLOG(1) << "Looking up coordinates for IP address: " << ip;
    Location location = { 0.0, 0.0, 0.0, 0.0, 0.0 };

    std::string latitude_str = db_->get_field(
        ip, "en", GeoLite2PP::VCStr{ "location", "latitude" });
    VLOG(1) << "Latitude string: " << latitude_str;
    std::string longitude_str = db_->get_field(
        ip, "en", GeoLite2PP::VCStr{ "location", "longitude" });
    VLOG(1) << "Longitude string: " << longitude_str;
    try {
        if (!latitude_str.empty()) {
            location.latitude = std::stod(latitude_str); // can throw
        }
        if (!longitude_str.empty()) {
            location.longitude = std::stod(longitude_str); // can throw
        }
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

// Given a reference to a vector of pointers to EdgeNode objects,
// this method will create a TreeData structure that holds a
// reference to the EdgeNode vector and constructs a new KD-Tree
// from the location data contained within the EdgeNodes.
// Returns a shared_ptr to the TreeData structure.
std::shared_ptr<TreeData> Geo::buildTreeData(const std::vector<std::shared_ptr<EdgeNode>>& nodes) {
    auto td = std::make_shared<TreeData>();
    td->cloud.pts = nodes;
    td->tree = std::make_shared<kd_tree_t>(
        3, td->cloud, KDTreeSingleIndexAdaptorParams(10));
    td->tree->buildIndex();
    return td;
}

// Queries the KD-Tree for the "n" nearest Locations
// to the provided Location "l". Returns a vector of
// the indices of EdgeNodes in the TreeData PointCloud
// class member. These indices can be used to lookup
// the actual EdgeNode objects in the NetworkState class.
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

// Sets the TreeData class member to a new TreeData instance
// in a thread-safe manner.
void Geo::setTreeData(std::shared_ptr<TreeData> treeData) {
    { // critical section
        treeData_ = treeData;
    }   
}
