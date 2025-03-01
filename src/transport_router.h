#pragma once

#include "router.h"
#include "transport_catalogue.h"

#include <map>
#include <memory>

namespace router {

class TransportRouter {
public:
    // static constexpr double SPEED_COEFF = 100 / 6;
    TransportRouter() = default;

    TransportRouter(const TransportCatalogue &db, int wait_time, double bus_velocity);

    void BuildGraph(const TransportCatalogue &db);
    const std::optional<graph::Router<double>::RouteInfo> CreateRoute(const std::string_view stop_from, const std::string_view stop_to) const;

private:
    int bus_wait_time_ = 0;
    double bus_velocity_ = 0.0;
    std::map<std::string, graph::VertexId> stop_ids_;
    graph::DirectedWeightedGraph<double> graph_;
    std::unique_ptr<graph::Router<double>> router_;

    void FillGraphWithVertices(const std::unordered_map<std::string_view, domain::Stop *> &stops_list,
                               graph::DirectedWeightedGraph<double> &stops_graph);

    void FillGraphWithEdges(const TransportCatalogue &db, const std::unordered_map<std::string_view, domain::Bus *> &bases_list,
                            graph::DirectedWeightedGraph<double> &stops_graph);
};

} // namespace router