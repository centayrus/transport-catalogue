#include <cstdint>
#include <map>
#include <sstream>

#include "map_renderer.h"

MapRenderer::MapRenderer(RenderSets rs) : render_sets_(rs) {}

svg::Polyline MapRenderer::MakeRenderPolyline(const StopToPoint &stop_point, const size_t &pallet_num) const {
    svg::Polyline line;
    const size_t pallet_count = render_sets_.color_palette.size();
    for (const auto &point : stop_point.point) {
        line.AddPoint(point.second);
    }
    line.SetFillColor("none")
        .SetStrokeColor(render_sets_.color_palette.at(pallet_num % pallet_count))
        .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
        .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND)
        .SetStrokeWidth(render_sets_.line_width);
    return line;
}

void MapRenderer::MakeRenderBusName(std::vector<svg::Text> &result, const StopToPoint &stop_point, size_t &pallet_num) const {
    svg::Text text1, text2;
    std::vector<svg::Point> point;
    size_t pallet_size = render_sets_.color_palette.size();
    auto point_num = stop_point.point.size();
    if (stop_point.is_roundtrip || stop_point.point.back().second == stop_point.point.at(point_num / 2).second) {
        point.push_back(stop_point.point.back().second);
    } else {
        point.push_back(stop_point.point.back().second);
        point.push_back(stop_point.point.at(point_num / 2).second);
    }
    for (const auto &pnt : point) {
        std::string str(stop_point.bus);
        text1.SetPosition(pnt)
            .SetData(str)
            .SetFillColor(render_sets_.underlayer_color)
            .SetStrokeColor(render_sets_.underlayer_color)
            .SetStrokeWidth(render_sets_.underlayer_width)
            .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
            .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND)
            .SetOffset(render_sets_.bus_label_offset)
            .SetFontSize(static_cast<uint32_t>(render_sets_.bus_label_font_size))
            .SetFontWeight("bold")
            .SetFontFamily("Verdana");
        result.push_back(text1);
        text2.SetPosition(pnt)
            .SetData(str)
            .SetFillColor(render_sets_.color_palette.at(pallet_num % pallet_size))
            .SetOffset(render_sets_.bus_label_offset)
            .SetFontSize(static_cast<uint32_t>(render_sets_.bus_label_font_size))
            .SetFontFamily("Verdana")
            .SetFontWeight("bold");
        result.push_back(text2);
    }
}

void MapRenderer::MakeRenderStopName(std::vector<svg::Text> &result) const {
    svg::Text text1, text2;
    for (const auto &item : unique_stops_) {
        std::string str(item.first);
        text1.SetPosition((item.second))
            .SetOffset(render_sets_.stop_label_offset)
            .SetFontSize(static_cast<uint32_t>(render_sets_.stop_label_font_size))
            .SetFontFamily("Verdana")
            .SetFillColor(render_sets_.underlayer_color)
            .SetStrokeColor(render_sets_.underlayer_color)
            .SetStrokeWidth(render_sets_.underlayer_width)
            .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
            .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND)
            .SetData(str);
        result.push_back(text1);
        text2.SetPosition((item.second))
            .SetOffset(render_sets_.stop_label_offset)
            .SetFontSize(static_cast<uint32_t>(render_sets_.stop_label_font_size))
            .SetFontFamily("Verdana")
            .SetFillColor("black")
            .SetData(str);
        result.push_back(text2);
    }
}

std::vector<svg::Circle> MapRenderer::MakeRenderPoints() const {
    std::vector<svg::Circle> result;
    std::map<std::string_view, svg::Point> temp_points;
    for (const auto &stop : stop_points_) {
        for (const auto &stop_point : stop.point) {
            temp_points[stop_point.first] = stop_point.second;
        }
    }
    for (const auto &item : temp_points) {
        svg::Circle dot;
        dot.SetCenter(item.second).SetRadius(GetSets().stop_radius).SetFillColor("white");
        result.push_back(dot);
    }
    return result;
}

void MapRenderer::SetStopPoint(const StopToPoint &stop_point) {
    stop_points_.push_back(stop_point);
}

const RenderSets &MapRenderer::GetSets() const {
    return render_sets_;
}

const std::vector<StopToPoint> &MapRenderer::GetStopPoints() const {
    return stop_points_;
}

void MapRenderer::DocRender(std::ostream &out) const {
    doc_.Render(out);
}

void MapRenderer::SetUniqStop(const StopItem stop_item) {
    unique_stops_ = std::move(stop_item);
}
