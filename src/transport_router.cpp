#include "transport_router.h"
#include "domain.h"

using namespace std::literals;
using namespace std::string_view_literals;

namespace router {

TransportRouter::TransportRouter(const TransportCatalogue &db, int wait_time, double bus_velocity)
    : bus_wait_time_(wait_time),
      bus_velocity_(bus_velocity) {
    BuildGraph(db);
    router_ = std::make_unique<graph::Router<double>>(graph_);
}

void TransportRouter::FillGraphWithVertices(const std::unordered_map<std::string_view, domain::Stop *> &stops_list,
                                            graph::DirectedWeightedGraph<double> &stops_graph) {

    std::map<std::string, graph::VertexId> stop_ids;
    graph::VertexId vertex_id = 0;
    // заполнение графа вершинами - ожиданиями
    for (const auto &[stop_name, stop_info] : stops_list) {
        stop_ids[stop_info->name] = vertex_id;
        stops_graph.AddEdge({stop_info->name,
                             0,
                             vertex_id,
                             ++vertex_id,
                             static_cast<double>(bus_wait_time_)});
        ++vertex_id;
    }
    stop_ids_ = std::move(stop_ids);
}

void TransportRouter::FillGraphWithEdges(const TransportCatalogue &db, const std::unordered_map<std::string_view, domain::Bus *> &bases_list,
                                         graph::DirectedWeightedGraph<double> &stops_graph) {
    // константное значение используется для перевода км/ч в м/мин
    const double speed_coeff = 100. / 6.;
    for (auto it = bases_list.begin(); it != bases_list.end(); ++it) {
        const auto &bus = it->second;
        const auto &stops = bus->stops;
        size_t stops_count = stops.size();
        // наполнение графа ребрами с весами
        for (size_t i = 0; i < stops_count; ++i) {
            for (size_t j = i + 1; j < stops_count; ++j) {
                const domain::Stop *stop_from = stops[i];
                const domain::Stop *stop_to = stops[j];
                double dist_total = 0;
                for (size_t k = i + 1; k <= j; ++k) {
                    dist_total += db.GetDistance(stops[k - 1], stops[k]);
                }
                stops_graph.AddEdge({bus->bus_route,
                                     j - i,
                                     stop_ids_.at(stop_from->name) + 1,
                                     stop_ids_.at(stop_to->name),
                                     static_cast<double>(dist_total) / (bus_velocity_ * speed_coeff)});
            }
        }
    }
}

void TransportRouter::BuildGraph(const TransportCatalogue &db) {

    const auto &all_stops_list = db.GetAllStopsList();
    graph::DirectedWeightedGraph<double> stops_graph(all_stops_list.size() * 2);
    /*
     * Здесь кажется вернуть данные в stops_graph через параметр правильнее,
     * а stop_ids_ инициализировать внутри метода, т.к. он относится только к вершинам.
     * Возвращать данные и через return и в параметре метода неправильно
     * */
    FillGraphWithVertices(all_stops_list, stops_graph);
    const auto &all_buses_list = db.GetAllRoutes();
    // аналогично возвращаем данные stops_graph из параметра
    FillGraphWithEdges(db, all_buses_list, stops_graph);
    graph_ = std::move(stops_graph);
}

const std::optional<graph::Router<double>::RouteInfo> TransportRouter::CreateRoute(const std::string_view stop_from, const std::string_view stop_to) const {
    return router_->BuildRoute(stop_ids_.at(std::string(stop_from)), stop_ids_.at(std::string(stop_to)));
}

} // namespace router