#pragma once
#include <string>
#include <variant>

#include "json.h"
#include "map_renderer.h"
#include "request_handler.h"
#include "transport_catalogue.h"

// форматирование статистики маршрутов в json
const json::Node BusStatLoad(const domain::BusStat bus_stat, const int req_id);
// форматирование статистики остановок в json
const json::Node StopStatLoad(const domain::StopStat stop_stat, const int req_id);
// перевод координат остановок в Point
void StopPointsSetter(const RequestHandler &req_handler, MapRenderer &renderer);
// заполнение остановок и маршрутов в каталог
void FillBusesAndStops(TransportCatalogue &db, const std::vector<json::Node> &array_copy);
// заполнение расстояний в каталоге
void FillRoadDistances(TransportCatalogue &db, const std::vector<json::Node> &array_copy);
// загрузка данных в справочник
void LoadCatalogue(TransportCatalogue &db, const std::vector<json::Node> &base_req);
// получение документа по запросам
json::Document GetReqsResults(const RequestHandler &req_handler, const std::vector<json::Node> &base_req, MapRenderer &renderer);

// заполнение атрибутами отрисовки
void FillRenderSets(const json::Node &render_node, RenderSets &render_sets);

// создание объектов ломаных
std::vector<svg::Polyline> MakePolylineMap(const MapRenderer &renderer);
// отрисовка названий маршрутов
std::vector<svg::Text> MakeBusNameTextMap(const MapRenderer &renderer);
// отрисовка названий остановок
std::vector<svg::Text> MakeStopNameTextMap(const MapRenderer &renderer);

// шаблонная функция для рендеринга типов объектов svg
template <typename Container>
void RenderSchema(std::vector<Container> &p, MapRenderer &renderer) {
    return renderer.Render(p);
}
// выставление точек остановок в соответствии с Point
std::vector<svg::Circle> SetDots(const MapRenderer &renderer);

// функция для вывода в поток объектов svg
void MakeSvg(std::ostream &out, const RequestHandler &req_handler, MapRenderer &renderer);

const json::Node RouteToNode(const std::optional<graph::Router<double>::RouteInfo>& route_data, const int req_id);