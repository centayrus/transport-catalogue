#include "json_builder.h"
#include <iostream>

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "json_reader.h"
#include "map_renderer.h"
#include "request_handler.h"

using namespace std;

int main() {
    std::ostringstream out;
    TransportCatalogue catalogue;

    const auto json = json::Load(std::cin);
    auto base_data = json.GetRoot().AsDict().at("base_requests"s).AsArray();      // вектор для заполнения базы
    auto base_req = json.GetRoot().AsDict().at("stat_requests"s).AsArray();       // вектор с запросами, в ответ на него возвращается статистика
    auto render_node = json.GetRoot().AsDict().at("render_settings"s).AsDict();   // свойства для отрисовки
    auto routing_node = json.GetRoot().AsDict().at("routing_settings"s).AsDict(); // свойство ожидания и скорости движения
                                                   
    LoadCatalogue(catalogue, base_data);
    // передаем в класс построения маршрута константную ссылку на каталог и мапу сеттингов

    router::TransportRouter router(catalogue, routing_node.at("bus_wait_time").AsInt(), routing_node.at("bus_velocity").AsDouble());

    RequestHandler req_handler(catalogue, router);
    RenderSets render_sets;
    FillRenderSets(render_node, render_sets);

    MapRenderer renderer(render_sets);

    json::Document doc = GetReqsResults(req_handler, base_req, renderer);
    json::Print(doc, out);

    std::cout << out.str();
}
