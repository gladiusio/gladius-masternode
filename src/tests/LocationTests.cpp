#include <gtest/gtest.h>

#include "Location.h"

TEST (Location, TestConvertToCartesian) {
    Location l;
    l.longitude = 20.0;
    l.latitude = 35.2;
    l.x = 0.0;
    l.y = 0.0;
    l.z = 0.0;
    // longitude in radians = (20.0 * π) / 180 = 0.3490658504
    // latitude in radians = (35.2 * π) / 180 = 0.6143558967
    // x = 6371 * cos(0.6143558967) * cos(0.3490658504) = 4892.068113
    // y = 6371 * cos(0.6143558967) * sin(0.3490658504) = 1780.567177
    // z = 6371 * sin(0.6143558967) = 3672.450286
    EXPECT_EQ(0.0, l.x);
    EXPECT_EQ(0.0, l.y);
    EXPECT_EQ(0.0, l.z);
    l.convertToCartesian();
    EXPECT_EQ(4892, trunc(l.x));
    EXPECT_EQ(1780, trunc(l.y));
    EXPECT_EQ(3672, trunc(l.z));
}
