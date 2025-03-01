#include <algorithm>
#include <set>
#include <sstream>
#include <vector>

#include "json_builder.h"
#include "json_reader.h"
#include "map_renderer.h"

void FillBusesAndStops(TransportCatalogue &db, const std::vector<json::Node> &array_copy) {
    for (const auto &item : array_copy) {
        if (item.AsDict().at("type").AsString() == "Stop") {
            db.AddStop(item.AsDict().at("name").AsString(), {item.AsDict().at("latitude").AsDouble(), item.AsDict().at("longitude").AsDouble()});
        } else {
            std::vector<std::string_view> string_vec;
            for (const auto &node1 : item.AsDict().at("stops").AsArray()) {
                string_vec.emplace_back(node1.AsString());
            }
            if (!item.AsDict().at("is_roundtrip").AsBool()) {
                string_vec.insert(string_vec.end(), std::next(string_vec.rbegin()), string_vec.rend());
            }
            db.AddBus(item.AsDict().at("name").AsString(), string_vec, item.AsDict().at("is_roundtrip").AsBool());
        }
    }
}

void FillRoadDistances(TransportCatalogue &db, const std::vector<json::Node> &array_copy) {
    for (const auto &stop : array_copy) {
        if (stop.AsDict().at("type").AsString() == "Stop") {
            if (stop.AsDict().count("road_distances")) {
                for (const auto &stop_list : stop.AsDict().at("road_distances").AsDict()) {
                    db.SetDistance(stop.AsDict().at("name").AsString(), stop_list.first, stop_list.second.AsInt());
                }
            }
        }
    }
}

void LoadCatalogue(TransportCatalogue &db, const std::vector<json::Node> &base_req) {
    std::vector<json::Node> array_copy;
    // копирование array, чтобы обойти константность при сортировке
    for (auto const &item : base_req) {
        if (item.AsDict().at("type").AsString() == "Stop")
            array_copy.push_back(item);
    }
    for (auto const &item : base_req) {
        if (item.AsDict().at("type").AsString() == "Bus")
            array_copy.push_back(item);
    }
    FillBusesAndStops(db, array_copy);
    FillRoadDistances(db, array_copy);
}

void MakeSvg(std::ostream &out, const RequestHandler &req_handler, MapRenderer &renderer) {
    StopPointsSetter(req_handler, renderer);

    auto polyline_set = MakePolylineMap(renderer);
    auto bus_text = MakeBusNameTextMap(renderer);
    auto dot = SetDots(renderer);
    auto stop_text = MakeStopNameTextMap(renderer);

    RenderSchema(polyline_set, renderer);
    RenderSchema(bus_text, renderer);
    RenderSchema(dot, renderer);
    RenderSchema(stop_text, renderer);

    renderer.DocRender(out);
}

json::Document GetReqsResults(const RequestHandler &req_handler, const std::vector<json::Node> &base_req, MapRenderer &renderer) {
    json::Node tmp_node;
    json::Array res_array;
    for (auto &req : base_req) {
        auto req_id = req.AsDict().at("id").AsInt();
        if (req.AsDict().at("type").AsString() == "Stop") {
            // формирование структуры статистики остановок и создание по нему json-объекта
            domain::StopStat stop_stat = req_handler.GetBusesByStop(req.AsDict().at("name").AsString());
            tmp_node = StopStatLoad(std::move(stop_stat), req_id);
        } else if (req.AsDict().at("type").AsString() == "Bus") {
            // формирование структуры статистики маршрутов и создание по нему json-объекта
            domain::BusStat bus_stat = req_handler.GetBusStat(req.AsDict().at("name").AsString());
            tmp_node = BusStatLoad(std::move(bus_stat), req_id);
        } else if (req.AsDict().at("type").AsString() == "Map") {
            // формирование svg-объекта в формате xml и передача их в json-объект
            std::ostringstream ostr;
            MakeSvg(ostr, req_handler, renderer);
            auto nod = ostr.str();
            tmp_node = json::Builder{}.StartDict().Key("map").Value(nod).Key("request_id").Value(req_id).EndDict().Build();
        } else if (req.AsDict().at("type").AsString() == "Route") {
            // формирование ответа по маршруту
            std::string_view from = req.AsDict().at("from").AsString();
            std::string_view to = req.AsDict().at("to").AsString();
            auto route_data = req_handler.GetOptimalRoute(from, to);
            tmp_node = RouteToNode(std::move(route_data), req_id);
        }
        res_array.push_back(tmp_node);
    }
    json::Document result_doc(std::move(res_array));
    return result_doc;
}

const json::Node RouteToNode(const std::optional<graph::Router<double>::RouteInfo> &route_data, const int req_id) {
    json::Node result;

    if (!route_data.has_value()) {
        result = json::Builder{}.StartDict().Key("error_message").Value("not found").Key("request_id").Value(req_id).EndDict().Build();
    } else {
        json::Array route_list;
        route_list.reserve(route_data.value().edges.size());
        double total_time = 0.;
        for (const auto &edge : route_data.value().edges) {
            // const auto edge_map = req_handler.GetRouterGraph().GetEdge(edge);
            // std::cout << *;
            auto edge_item = *edge.second;
            if (edge_item.span == 0) {
                route_list.emplace_back(json::Node(json::Builder{}.StartDict().Key("stop_name").Value(edge_item.name).Key("time").Value(edge_item.weight).Key("type").Value("Wait").EndDict().Build()));
                total_time += edge_item.weight;
            } else {
                route_list.emplace_back(json::Node(json::Builder{}.StartDict().Key("bus").Value(edge_item.name).Key("span_count").Value(static_cast<int>(edge_item.span)).Key("time").Value(edge_item.weight).Key("type").Value("Bus").EndDict().Build()));
                total_time += edge_item.weight;
            }
        }
        result = json::Builder{}.StartDict().Key("items").Value(route_list).Key("request_id").Value(req_id).Key("total_time").Value(total_time).EndDict().Build();
    }
    return result;
}

const json::Node BusStatLoad(const domain::BusStat bus_stat, const int req_id) {
    json::Node result;
    if (!bus_stat) {
        result = json::Builder{}.StartDict().Key("error_message").Value("not found").Key("request_id").Value(req_id).EndDict().Build();
    } else {
        result = json::Builder{}.StartDict().Key("curvature").Value(bus_stat.dist_proportion).Key("route_length").Value(bus_stat.total_distance).Key("stop_count").Value(bus_stat.stop_count).Key("unique_stop_count").Value(bus_stat.uniq_stops).Key("request_id").Value(req_id).EndDict().Build();
    }
    return result;
}

const json::Node StopStatLoad(const domain::StopStat stop_stat, const int req_id) {
    json::Node result;
    json::Array tmp_array;
    if (!stop_stat) {
        result = json::Builder{}.StartDict().Key("error_message").Value("not found").Key("request_id").Value(req_id).EndDict().Build();
    } else {
        for (const auto *bus : stop_stat.bus_routes) {
            tmp_array.push_back(bus->bus_route);
        }
        result = json::Builder{}.StartDict().Key("request_id").Value(req_id).Key("buses").Value(tmp_array).EndDict().Build();
    }
    return result;
}

void FillRenderSets(const json::Node &render_node, RenderSets &render_sets) {
    json::Dict attrs = render_node.AsDict();
    render_sets.width = attrs.at("width").AsDouble();
    render_sets.height = attrs.at("height").AsDouble();
    render_sets.padding = attrs.at("padding").AsDouble();
    render_sets.line_width = attrs.at("line_width").AsDouble();
    render_sets.stop_radius = attrs.at("stop_radius").AsDouble();
    render_sets.bus_label_font_size = attrs.at("bus_label_font_size").AsInt();
    render_sets.bus_label_offset = svg::Point(attrs.at("bus_label_offset").AsArray()[0].AsDouble(), attrs.at("bus_label_offset").AsArray()[1].AsDouble());
    render_sets.stop_label_font_size = attrs.at("stop_label_font_size").AsInt();
    render_sets.stop_label_offset = svg::Point(attrs.at("stop_label_offset").AsArray()[0].AsDouble(), attrs.at("stop_label_offset").AsArray()[1].AsDouble());
    render_sets.underlayer_width = attrs.at("underlayer_width").AsDouble();
    if (attrs.at("underlayer_color").IsArray() && attrs.at("underlayer_color").AsArray().size() == 4) {
        render_sets.underlayer_color = svg::Rgba(static_cast<uint8_t>(attrs.at("underlayer_color").AsArray()[0].AsInt()),
                                                 static_cast<uint8_t>(attrs.at("underlayer_color").AsArray()[1].AsInt()),
                                                 static_cast<uint8_t>(attrs.at("underlayer_color").AsArray()[2].AsInt()),
                                                 attrs.at("underlayer_color").AsArray()[3].AsDouble());
    } else if (attrs.at("underlayer_color").IsString()) {
        render_sets.underlayer_color = attrs.at("underlayer_color").AsString();
    } else {
        render_sets.underlayer_color = svg::Rgb(static_cast<uint8_t>(attrs.at("underlayer_color").AsArray()[0].AsInt()),
                                                static_cast<uint8_t>(attrs.at("underlayer_color").AsArray()[1].AsInt()),
                                                static_cast<uint8_t>(attrs.at("underlayer_color").AsArray()[2].AsInt()));
    }
    for (const auto &item : attrs.at("color_palette").AsArray()) {
        if (item.IsString()) {
            render_sets.color_palette.push_back(item.AsString());
        } else if (item.IsArray() && item.AsArray().size() == 3) {
            auto rgb = svg::Rgb(static_cast<uint8_t>(item.AsArray()[0].AsInt()),
                                static_cast<uint8_t>(item.AsArray()[1].AsInt()),
                                static_cast<uint8_t>(item.AsArray()[2].AsInt()));
            render_sets.color_palette.push_back(rgb);
        } else {
            auto rgba = svg::Rgba(static_cast<uint8_t>(item.AsArray()[0].AsInt()),
                                  static_cast<uint8_t>(item.AsArray()[1].AsInt()),
                                  static_cast<uint8_t>(item.AsArray()[2].AsInt()),
                                  item.AsArray()[3].AsDouble());
            render_sets.color_palette.push_back(rgba);
        }
    }
    if (!render_sets.validateRenderSets()) {
        throw std::invalid_argument("Render set attrs is not valid");
    }
}

void StopPointsSetter(const RequestHandler &req_handler, MapRenderer &renderer) {
    std::unordered_map<std::string_view, domain::Bus *> buses = req_handler.GetAllBusRoutes();
    std::vector<geo::Coordinates> coordinate_pool;
    for (const auto &bus : buses) {
        for (const auto &stop : (*bus.second).stops) {
            coordinate_pool.emplace_back((*stop).coordinate);
        }
    }
    std::vector<svg::Polyline> result;
    // создание объекта для перевода географических координат в точки на плоскости
    SphereProjector point_mapper(coordinate_pool.begin(), coordinate_pool.end(), renderer.GetSets().width, renderer.GetSets().height, renderer.GetSets().padding);
    std::map sorted_routes(buses.begin(), buses.end());
    StopItem uniq_stops;
    for (const auto &bus : sorted_routes) {
        std::vector<std::pair<std::string_view, svg::Point>> tmp_points;
        for (const auto &stop : (*bus.second).stops) {
            tmp_points.push_back({(*stop).name, point_mapper((*stop).coordinate)});
            uniq_stops[tmp_points.back().first] = std::move(tmp_points.back().second);
        }
        renderer.SetStopPoint({bus.first, tmp_points, bus.second->is_roundtrip});
        renderer.SetUniqStop(uniq_stops);
    }
}

std::vector<svg::Polyline> MakePolylineMap(const MapRenderer &renderer) {
    size_t pallet_num = 0;
    std::vector<svg::Polyline> result;
    for (const auto &stop_point : renderer.GetStopPoints()) {
        result.emplace_back(renderer.MakeRenderPolyline(stop_point, pallet_num));
        ++pallet_num;
    }
    return result;
}

std::vector<svg::Text> MakeBusNameTextMap(const MapRenderer &renderer) {
    size_t pallet_num = 0;
    std::vector<svg::Text> result;
    for (const auto &stop_point : renderer.GetStopPoints()) {
        renderer.MakeRenderBusName(result, stop_point, pallet_num);
        ++pallet_num;
    }
    return result;
}

std::vector<svg::Text> MakeStopNameTextMap(const MapRenderer &renderer) {
    std::vector<svg::Text> result;
    renderer.MakeRenderStopName(result /* , pallet_num */);
    return result;
}

std::vector<svg::Circle> SetDots(const MapRenderer &renderer) {
    return renderer.MakeRenderPoints();
}
