#pragma once

#include <cstdint>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

using namespace std::literals;
namespace svg {

struct Rgb {
    // public:
    Rgb() = default;
    Rgb(uint8_t r, uint8_t g, uint8_t b) : red(r), green(g), blue(b) {}

    // private:
    uint8_t red = 0;
    uint8_t green = 0;
    uint8_t blue = 0;
};

struct Rgba {
    // public:
    Rgba() = default;
    Rgba(uint8_t r, uint8_t g, uint8_t b, double o) : red(r), green(g), blue(b), opacity(o) {}

    // private:
    uint8_t red = 0;
    uint8_t green = 0;
    uint8_t blue = 0;
    double opacity = 1.0;
};

using Color = std::variant<std::monostate, std::string, svg::Rgb, svg::Rgba>;

// Объявив в заголовочном файле константу со спецификатором inline,
// мы сделаем так, что она будет одной на все единицы трансляции,
// которые подключают этот заголовок.
// В противном случае каждая единица трансляции будет использовать свою копию этой константы
inline const Color NoneColor{"none"};

enum class StrokeLineCap {
    BUTT,
    ROUND,
    SQUARE,
};

enum class StrokeLineJoin {
    ARCS,
    BEVEL,
    MITER,
    MITER_CLIP,
    ROUND,
};

struct Point {
    Point() = default;
    Point(double x1, double y1)
        : x(x1), y(y1) {
    }
    bool operator==(const Point &p) const {
        return (*this).x == p.x || (*this).y == p.y;
    }
    double x = 0;
    double y = 0;
};

/*
 * Вспомогательная структура, хранящая контекст для вывода SVG-документа с отступами.
 * Хранит ссылку на поток вывода, текущее значение и шаг отступа при выводе элемента
 */
struct RenderContext {
    RenderContext(std::ostream &out1)
        : out(out1) {
    }

    RenderContext(std::ostream &out1, int indent_step1, int indent1 = 0)
        : out(out1), indent_step(indent_step1), indent(indent1) {
    }

    RenderContext Indented() const {
        return {out, indent_step, indent + indent_step};
    }

    void RenderIndent() const {
        for (int i = 0; i < indent; ++i) {
            out.put(' ');
        }
    }

    std::ostream &out;
    int indent_step = 0;
    int indent = 0;
};

std::ostream &operator<<(std::ostream &out, const svg::StrokeLineCap &line_cap);

std::ostream &operator<<(std::ostream &out, const svg::StrokeLineJoin &line_join);

struct OstreamColorPrint {
    std::ostream &out;
    void operator()(std::monostate) const {
        out << "none"sv;
    }
    void operator()(std::string col) const {
        out << col;
    }
    void operator()(svg::Rgb col) const {
        out << "rgb("sv << static_cast<unsigned int>(col.red) << ","sv << static_cast<unsigned int>(col.green) << ","sv << static_cast<unsigned int>(col.blue) << ")"sv;
    }
    void operator()(svg::Rgba col) const {
        out << "rgba("sv << static_cast<unsigned int>(col.red) << ","sv << static_cast<unsigned int>(col.green) << ","sv << static_cast<unsigned int>(col.blue) << "," << col.opacity << ")"sv;
    }
};

std::ostream &operator<<(std::ostream &out, const svg::Color &col);

template <typename Owner>
class PathProps {
public:
    Owner &SetFillColor(Color fill_color) {
        fill_color_ = std::move(fill_color);
        return AsOwner();
    }

    Owner &SetStrokeColor(Color stroke_color) {
        stroke_color_ = std::move(stroke_color);
        return AsOwner();
    }

    Owner &SetStrokeWidth(double width) {
        width_ = std::move(width);
        return AsOwner();
    }

    Owner &SetStrokeLineCap(StrokeLineCap line_cap) {
        line_cap_ = std::move(line_cap);
        return AsOwner();
    }

    Owner &SetStrokeLineJoin(StrokeLineJoin line_join) {
        line_join_ = std::move(line_join);
        return AsOwner();
    }

protected:
    ~PathProps() = default;

    void RenderAttrs(std::ostream &out) const {
        using namespace std::literals;
        if (fill_color_) {
            out << " fill=\""sv << *fill_color_ << "\""sv;
        }
        if (stroke_color_) {
            out << " stroke=\""sv << *stroke_color_ << "\""sv;
        }
        if (width_) {
            out << " stroke-width=\""sv << *width_ << "\""sv;
        }
        if (line_cap_.has_value()) {
            out << " stroke-linecap=\""sv << *line_cap_ << "\""sv;
        }
        if (line_join_.has_value()) {
            out << " stroke-linejoin=\""sv << *line_join_ << "\""sv;
        }
    }

private:
    std::optional<Color> fill_color_;
    std::optional<Color> stroke_color_;
    std::optional<double> width_;
    std::optional<StrokeLineCap> line_cap_;
    std::optional<StrokeLineJoin> line_join_;

    Owner &AsOwner() {
        return static_cast<Owner &>(*this);
    }
};

class Object;

class ObjectContainer {
public:
    ObjectContainer() = default;

    template <typename Object>
    void Add(Object object);

    // Добавляет в svg-документ объект-наследник svg::Object
    virtual void AddPtr(std::unique_ptr<svg::Object> &&obj) = 0;

protected:
    ~ObjectContainer() = default;
    std::vector<std::unique_ptr<Object>> objects_ptr_;
};

class Drawable {
public:
    virtual void Draw(ObjectContainer &obj_container) const = 0;

    virtual ~Drawable() = default;
};

/*
 * Абстрактный базовый класс Object служит для унифицированного хранения
 * конкретных тегов SVG-документа
 * Реализует паттерн "Шаблонный метод" для вывода содержимого тега
 */
class Object {
public:
    void Render(const RenderContext &context) const;

    virtual ~Object() = default;

private:
    virtual void RenderObject(const RenderContext &context) const = 0;
};

/*
 * Класс Circle моделирует элемент <circle> для отображения круга
 * https://developer.mozilla.org/en-US/docs/Web/SVG/Element/circle
 */
class Circle final : public Object, public PathProps<Circle> {
public:
    Circle() = default;
    Circle &SetCenter(Point center);
    Circle &SetRadius(double radius);

private:
    void RenderObject(const RenderContext &context) const override;

    Point center_ = {0.0, 0.0};
    double radius_ = 1.0;
};

/*
 * Класс Polyline моделирует элемент <polyline> для отображения ломаных линий
 * https://developer.mozilla.org/en-US/docs/Web/SVG/Element/polyline
 */
class Polyline : public Object, public PathProps<Polyline> {
public:
    Polyline() = default;
    // Добавляет очередную вершину к ломаной линии
    Polyline &AddPoint(Point point);

private:
    void RenderObject(const RenderContext &context) const override;

    std::vector<Point> line_points_;
};

/*
 * Класс Text моделирует элемент <text> для отображения текста
 * https://developer.mozilla.org/en-US/docs/Web/SVG/Element/text
 */
class Text : public Object, public PathProps<Text> {
public:
    Text() = default;
    // Задаёт координаты опорной точки (атрибуты x и y)
    Text &SetPosition(Point pos);

    // Задаёт смещение относительно опорной точки (атрибуты dx, dy)
    Text &SetOffset(Point offset);

    // Задаёт размеры шрифта (атрибут font-size)
    Text &SetFontSize(uint32_t size);

    // Задаёт название шрифта (атрибут font-family)
    Text &SetFontFamily(std::string font_family);

    // Задаёт толщину шрифта (атрибут font-weight)
    Text &SetFontWeight(std::string font_weight);

    // Задаёт текстовое содержимое объекта (отображается внутри тега text)
    Text &SetData(std::string data);

private:
    void RenderObject(const RenderContext &context) const override;

    Point pos_ = {0.0, 0.0};
    Point offset_ = {0.0, 0.0};
    uint32_t size_ = 1;
    std::string font_family_ = ""s;
    std::string font_weight_ = ""s;
    std::string data_ = ""s;
};

class Document : public ObjectContainer {
public:
    /*
     Метод Add добавляет в svg-документ любой объект-наследник svg::Object.
     Пример использования:
     Document doc;
     doc.Add(Circle().SetCenter({20, 30}).SetRadius(15));*/

    // Добавляет в svg-документ объект-наследник svg::Object
    void AddPtr(std::unique_ptr<svg::Object> &&obj) override;

    // Выводит в ostream svg-представление документа
    void Render(std::ostream &out) const;

};

template <typename Object>
void ObjectContainer::Add(Object object) {
    objects_ptr_.emplace_back(std::make_unique<Object>(std::move(object)));
}

} // namespace svg
