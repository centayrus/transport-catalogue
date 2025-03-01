#include "transport_catalogue.h"

#include <algorithm>
#include <iomanip>
#include <utility>

// Добавление остановки в базу
void TransportCatalogue::AddStop(const std::string &stop_name, const geo::Coordinates &coordinate) {
    stops_list_.push_back({stop_name, coordinate});
    auto stop_ptr = &stops_list_.back();
    stopname_to_stop_[stops_list_.back().name] = stop_ptr;
}

// Добавление маршрута в базу
void TransportCatalogue::AddBus(const std::string &bus_name, const std::vector<std::string_view> &route, const bool &is_roundtrip) {
    std::vector<const domain::Stop *> stop_list_for_bus;
    for (const auto &stop : route) {
        if (stopname_to_stop_.count(stop)) {
            domain::Stop *stop_ptr = stopname_to_stop_.at(stop);
            stop_list_for_bus.push_back(stop_ptr);
        }
    }
    // маршрут и список остановок
    bus_routes_.push_back({bus_name, stop_list_for_bus, is_roundtrip});
    // ссылка на имя маршрута и дек маршрута-остновок
    busname_to_bus_.insert({bus_routes_.back().bus_route, &bus_routes_.back()});
    // Заполнение мапы для статистики остановок
    if (&bus_routes_.back()) {
        for (const auto &stops : bus_routes_.back().stops) {
            stopname_to_bus_[stops->name].insert(&bus_routes_.back());
        }
    }
}

// Статистика маршрута
domain::BusStat TransportCatalogue::ReportBusStatistic(std::string_view request) const {
    auto bus_pos = busname_to_bus_.find(request);
    if (bus_pos == busname_to_bus_.end()) {
        return {};
    }
    domain::Bus bus = *(*bus_pos).second;
    int common_stops_count = static_cast<int>(bus.stops.size());
    bool first_cycle = true;
    double total_distance = 0.;
    double route_distance = 0.;
    geo::Coordinates prev_location;
    const domain::Stop *prev_stop;
    std::unordered_set<std::string_view> uniq_stops;
    for (const auto *stop_item : bus.stops) {
        uniq_stops.insert(stop_item->name);
    }
    int uniq_stops_count = static_cast<int>(uniq_stops.size());
    // Рассчет дистанции маршрута
    for (const auto stop : bus.stops) {
        if (!first_cycle) {
            total_distance += geo::ComputeDistance(prev_location, stop->coordinate);
            route_distance += GetDistance(std::move(prev_stop), std::move(stop));
        }
        first_cycle = false;
        prev_location = stop->coordinate;
        prev_stop = stop;
    }
    return {common_stops_count, uniq_stops_count, route_distance, route_distance / total_distance};
}

// Информация по остановке
domain::StopStat TransportCatalogue::ReportStopStatistic(std::string_view stopname) const {
    std::string_view stop;
    if (stopname_to_stop_.count(stopname)) {
        stop = stopname;
    }
    auto stop_stat_pos = stopname_to_bus_.find(stopname);
    std::vector<const domain::Bus *> v_stop_stat;
    if (stop_stat_pos != stopname_to_bus_.end()) {
        v_stop_stat.reserve((*stop_stat_pos).second.size());
        v_stop_stat = {(*stop_stat_pos).second.begin(), (*stop_stat_pos).second.end()};
    }
    // Сортировка вектора для корректного вывода статистики
    std::sort(v_stop_stat.begin(), v_stop_stat.end(), [](const domain::Bus *lhs, const domain::Bus *rhs) {
        return std::lexicographical_compare((*lhs).bus_route.begin(), (*lhs).bus_route.end(),
                                            (*rhs).bus_route.begin(), (*rhs).bus_route.end());
    });
    return {stop, v_stop_stat};
}

double TransportCatalogue::GetDistance(const domain::Stop *prev_stop, const domain::Stop *cur_stop) const {
    std::pair<const domain::Stop *, const domain::Stop *> key = std::make_pair(prev_stop, cur_stop);
    auto value = stop_to_stop_dist_.find(key);
    if (value == stop_to_stop_dist_.end()) {
        key = std::make_pair(cur_stop, prev_stop);
        value = stop_to_stop_dist_.find(key);
    }
    return value->second;
}

void TransportCatalogue::SetDistance(const std::string_view a_name, const std::string_view b_name, const double &dist) {
    auto a_stop_ptr = stopname_to_stop_.find(a_name)->second;
    auto b_stop_ptr = stopname_to_stop_.find(b_name)->second;
    stop_to_stop_dist_[{a_stop_ptr, b_stop_ptr}] = dist;
}

const std::unordered_map<std::string_view, domain::Bus *> &TransportCatalogue::GetAllRoutes() const {
    return busname_to_bus_;
}

const std::unordered_map<std::string_view, domain::Stop *>& TransportCatalogue::GetAllStopsList() const {
    return stopname_to_stop_;
}
