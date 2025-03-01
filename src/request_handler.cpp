#include "request_handler.h"

RequestHandler::RequestHandler(const TransportCatalogue &db, router::TransportRouter &router) : db_(db), router_(router) {}

domain::BusStat RequestHandler::GetBusStat(const std::string_view &bus_name) const {
    return db_.ReportBusStatistic(bus_name);
}

domain::StopStat RequestHandler::GetBusesByStop(const std::string_view &stop_name) const {
    return db_.ReportStopStatistic(stop_name);
}

const std::optional<graph::Router<double>::RouteInfo> RequestHandler::GetOptimalRoute(const std::string_view stop_from, const std::string_view stop_to) const {
    return router_.CreateRoute(stop_from, stop_to);
}

std::unordered_map<std::string_view, domain::Bus *> RequestHandler::GetAllBusRoutes() const {
    return db_.GetAllRoutes();
}

