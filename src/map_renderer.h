#pragma once
#include "domain.h"
#include "geo.h"
#include "svg.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <utility>
#include <vector>

inline const double EPSILON = 1e-6;
inline bool IsZero(double value) {
    return std::abs(value) < EPSILON;
}

class SphereProjector {
public:
    // points_begin и points_end задают начало и конец интервала элементов geo::Coordinates
    template <typename PointInputIt>
    SphereProjector(PointInputIt points_begin, PointInputIt points_end,
                    double max_width, double max_height, double padding)
        : padding_(padding) //
    {
        // Если точки поверхности сферы не заданы, вычислять нечего
        if (points_begin == points_end) {
            return;
        }

        // Находим точки с минимальной и максимальной долготой
        const auto [left_it, right_it] = std::minmax_element(
            points_begin, points_end,
            [](auto lhs, auto rhs) { return lhs.lng < rhs.lng; });
        min_lon_ = left_it->lng;
        const double max_lon = right_it->lng;

        // Находим точки с минимальной и максимальной широтой
        const auto [bottom_it, top_it] = std::minmax_element(
            points_begin, points_end,
            [](auto lhs, auto rhs) { return lhs.lat < rhs.lat; });
        const double min_lat = bottom_it->lat;
        max_lat_ = top_it->lat;

        // Вычисляем коэффициент масштабирования вдоль координаты x
        std::optional<double> width_zoom;
        if (!IsZero(max_lon - min_lon_)) {
            width_zoom = (max_width - 2 * padding) / (max_lon - min_lon_);
        }

        // Вычисляем коэффициент масштабирования вдоль координаты y
        std::optional<double> height_zoom;
        if (!IsZero(max_lat_ - min_lat)) {
            height_zoom = (max_height - 2 * padding) / (max_lat_ - min_lat);
        }

        if (width_zoom && height_zoom) {
            // Коэффициенты масштабирования по ширине и высоте ненулевые,
            // берём минимальный из них
            zoom_coeff_ = std::min(*width_zoom, *height_zoom);
        } else if (width_zoom) {
            // Коэффициент масштабирования по ширине ненулевой, используем его
            zoom_coeff_ = *width_zoom;
        } else if (height_zoom) {
            // Коэффициент масштабирования по высоте ненулевой, используем его
            zoom_coeff_ = *height_zoom;
        }
    }

    // Проецирует широту и долготу в координаты внутри SVG-изображения
    svg::Point operator()(geo::Coordinates coords) const {
        return {
            (coords.lng - min_lon_) * zoom_coeff_ + padding_,
            (max_lat_ - coords.lat) * zoom_coeff_ + padding_};
    }

private:
    double padding_;
    double min_lon_ = 0.;
    double max_lat_ = 0.;
    double zoom_coeff_ = 0.;
};

// значения свойств объектов svg
struct RenderSets {
    double width = 0.;
    double height = 0.;
    double padding = 0.;
    double stop_radius = 0.;
    double line_width = 0.;
    int bus_label_font_size = 0;
    svg::Point bus_label_offset = {0., 0.};
    int stop_label_font_size = 0;
    svg::Point stop_label_offset = {0., 0.};
    svg::Color underlayer_color;
    double underlayer_width = 0.;
    std::vector<svg::Color> color_palette;
    bool validateRenderSets() const {
        return validateWidth() && validateHeight() 
        && validatePadding() && validateStopRadius() 
        && validateBusLabelFontSize() && validateBusLabelOffset() 
        && validateStopLabelFontSize() && validateStopLabelOffset() 
        && validateUnderlayerWidth();
    }

private:
    // валидация заполняемых данных
    bool validateWidth() const {
        return !(width < 0 || width > 100000.);
    }
    bool validateHeight() const {
        return !(height < 0 || height > 100000.);
    }
    bool validatePadding() const {
        return !(padding < 0 && padding > std::min(width, height) / 2);
    }
    bool validateStopRadius() const {
        return !(stop_radius < 0 || stop_radius > 100000);
    }
    bool validateBusLabelFontSize() const {
        return !(bus_label_font_size < 0. || bus_label_font_size > 100000.);
    }
    bool validateBusLabelOffset() const {
        return !(bus_label_offset.x < -100000. || bus_label_offset.y > 100000. || bus_label_offset.x < -100000. || bus_label_offset.y > 100000.);
    }
    bool validateStopLabelFontSize() const {
        return !(stop_label_font_size < 0 || stop_label_font_size > 100000.);
    }
    bool validateStopLabelOffset() const {
        return !(stop_label_offset.x < -100000. || stop_label_offset.y > 100000. || stop_label_offset.x < -100000. || stop_label_offset.y > 100000.);
    }
    bool validateUnderlayerWidth() const {
        return !(underlayer_width < 0. || underlayer_width > 100000.);
    }
};

using StopItem = std::map<std::string_view, svg::Point>;

// объект для соотношения маршрута - остановки и его координаты Point
struct StopToPoint {
    std::string_view bus;
    std::vector<std::pair<std::string_view, svg::Point>> point;
    bool is_roundtrip;
};

class MapRenderer {
public:
    MapRenderer() = default;
    MapRenderer(RenderSets rs);

    // рендеринг ломаной
    svg::Polyline MakeRenderPolyline(const StopToPoint &stop_point, const size_t &pallet_num) const;
    // рендеринг названий маршрута на конечных
    void MakeRenderBusName(std::vector<svg::Text> &result, const StopToPoint &stop_point, size_t &pallet_num) const;
    // рендеринг остановок
    void MakeRenderStopName(std::vector<svg::Text> &result) const;

    template <typename Container>
    void Render(std::vector<Container> &obj) {
        for (auto line : obj) {
            doc_.Add(std::move(line));
        }
    }
    // задание точек маршрута
    std::vector<svg::Circle> MakeRenderPoints() const;
    // заполнение остановками поля класса
    void SetStopPoint(const StopToPoint &stop_point);

    const RenderSets &GetSets() const;

    const std::vector<StopToPoint> &GetStopPoints() const;

    // вывод в поток out объектов svg
    void DocRender(std::ostream &out) const;
    // получение уникальных остановок
    void SetUniqStop(const StopItem stop_item);

private:
    RenderSets render_sets_;
    std::vector<StopToPoint> stop_points_;
    svg::Document doc_;
    StopItem unique_stops_;
};
