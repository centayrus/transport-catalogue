#pragma once

#include <cmath>

namespace geo {
struct Coordinates {
    double lat;
    double lng;
    bool operator==(const Coordinates &other) const {
        return lat == other.lat && lng == other.lng;
    }
    bool operator!=(const Coordinates &other) const {
        return !(*this == other);
    }
    bool operator<(const Coordinates &other) const {
        return lat < other.lat && lng < other.lng;
    }
};

double ComputeDistance(Coordinates from, Coordinates to);
} // namespace geo