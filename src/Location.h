#pragma once

#include <cmath>

const int EARTH_RADIUS = 6371; // km

struct Location {
    double latitude;
    double longitude;

    double x, y, z;

    void convertToCartesian() {
        double lat_rads = (latitude * M_PI) / 180;
        double long_rads = (longitude * M_PI) / 180;
        x = EARTH_RADIUS * cos(lat_rads) * cos(long_rads);
        y = EARTH_RADIUS * cos(lat_rads) * sin(long_rads);
        z = EARTH_RADIUS * sin(lat_rads);
    };
};
