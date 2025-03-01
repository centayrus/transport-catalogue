#pragma once
#include <deque>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "domain.h"
#include "geo.h"

class TransportCatalogue {

public:
    void AddStop(const std::string &name, const geo::Coordinates &coordinate);

    void AddBus(const std::string &bus_name, const std::vector<std::string_view> &route, const bool &is_roundtrip);

    domain::BusStat ReportBusStatistic(std::string_view request) const;

    domain::StopStat ReportStopStatistic(std::string_view stopname) const;

    void SetDistance(const std::string_view a_name, const std::string_view b_name, const double &dist);

    const std::unordered_map<std::string_view, domain::Bus *> &GetAllRoutes() const;

    double GetDistance(const domain::Stop *prev_stop, const domain::Stop *cur_stop) const;

    const std::unordered_map<std::string_view, domain::Stop *> &GetAllStopsList() const;
    
private:
    std::deque<domain::Stop> stops_list_;
    std::unordered_map<std::string_view, domain::Stop *> stopname_to_stop_;
    std::deque<domain::Bus> bus_routes_;
    std::unordered_map<std::string_view, domain::Bus *> busname_to_bus_;
    std::unordered_map<std::string_view, std::unordered_set<const domain::Bus *>> stopname_to_bus_;
    std::unordered_map<std::pair<const domain::Stop *, const domain::Stop *>, double, domain::StopDistanceHasher> stop_to_stop_dist_;
};
