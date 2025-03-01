#pragma once

#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

#include "geo.h"

namespace domain {

struct Stop;

class StopDistanceHasher {
public:
    // создание хеша
    size_t operator()(const std::pair<const Stop *, const Stop *> &interval) const;

private:
    // Комбинируем два хеша в один
    std::size_t combineHashes(const std::size_t h1, const std::size_t h2) const;
};

struct Stop {
    std::string name;
    geo::Coordinates coordinate;
};

struct Bus {
    std::string bus_route;
    std::vector<const Stop *> stops;
    bool is_roundtrip;
};

struct BusStat {
    explicit operator bool() const {
        return (stop_count) ? true : false;
    }

    bool operator!() const {
        return !operator bool();
    }
    int stop_count;
    int uniq_stops;
    double total_distance;
    double dist_proportion;
};

struct StopStat {
    explicit operator bool() const {
        return (!name.empty());
    }

    bool operator!() const {
        return !operator bool();
    }
    std::string_view name;
    std::vector<const Bus *> bus_routes;
};

} // namespace domain