#pragma once

#include <cmath>

const int EARTH_RADIUS = 6371; // km

struct Location {
    double latitude;
    double longitude;

    double x, y, z;

    // mutator function that sets the x, y, and z members
    // as the Cartesian coordinates converted from lat/long members
    void convertToCartesian() {
        double lat_rads = (latitude * M_PI) / 180;
        double long_rads = (longitude * M_PI) / 180;
        x = EARTH_RADIUS * cos(lat_rads) * cos(long_rads);
        y = EARTH_RADIUS * cos(lat_rads) * sin(long_rads);
        z = EARTH_RADIUS * sin(lat_rads);
    };
};
