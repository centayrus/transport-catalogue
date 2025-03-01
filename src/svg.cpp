#include "svg.h"

#include <iomanip>

namespace svg {

using namespace std::literals;

void Object::Render(const RenderContext &context) const {
    context.RenderIndent();

    // Делегируем вывод тега своим подклассам
    RenderObject(context);

    context.out << std::endl;
}

// ---------- Circle ------------------
Circle &Circle::SetCenter(Point center) {
    center_ = std::move(center);
    return *this;
}

Circle &Circle::SetRadius(double radius) {
    radius_ = std::move(radius);
    return *this;
}

void Circle::RenderObject(const RenderContext &context) const {
    auto &out = context.out;
    out << "<circle cx=\""sv << center_.x << "\" cy=\""sv << center_.y << "\" "sv;
    out << "r=\""sv << radius_ << "\""sv;
    RenderAttrs(context.out);
    out << "/>"sv;
}
// ---------- End Circle ------------------

// ---------- Polyline ------------------
Polyline &Polyline::AddPoint(Point point) {
    line_points_.emplace_back(point);
    return *this;
}

void Polyline::RenderObject(const RenderContext &context) const {
    auto &out = context.out;
    // std::string point_line = ""s;
    bool first = true;
    out << "<polyline points=\""sv;
    for (auto dot : line_points_) {
        if (!first) {
            out << ' ';
        }
        first = false;
        out << dot.x;
        out << ","sv;
        out << dot.y;
    }
    out << "\""sv;
    RenderAttrs(context.out);
    out << "/>"sv;
}
// ---------- End Polyline ------------------

// ---------- Text ------------------
Text &Text::SetPosition(Point pos) {
    pos_ = std::move(pos);
    return *this;
}

Text &Text::SetOffset(Point offset) {
    offset_ = std::move(offset);
    return *this;
}

Text &Text::SetFontSize(uint32_t size) {
    size_ = std::move(size);
    return *this;
}

Text &Text::SetFontFamily(std::string font_family) {
    font_family_ = std::move(font_family);
    return *this;
}

Text &Text::SetFontWeight(std::string font_weight) {
    font_weight_ = std::move(font_weight);
    return *this;
}

Text &Text::SetData(std::string data) {
    data_ = std::move(data);
    return *this;
}

void Text::RenderObject(const RenderContext &context) const {
    auto &out = context.out;
    std::string type;
    out << "<text"sv;
    RenderAttrs(context.out);
    out << " x=\""sv << pos_.x << "\" y=\""sv << pos_.y << "\" "sv;
    out << "dx=\""sv << offset_.x << "\" dy=\""sv << offset_.y << "\" "sv;
    out << "font-size=\""sv << size_ << "\""sv;
    if (!font_family_.empty()) {
        out << " font-family=\""sv << font_family_ << "\""sv;
    }
    if (!font_weight_.empty()) {
        out << " font-weight=\""sv << font_weight_ << "\""sv;
    }
    out << ">"sv;
    if (data_.size()) {
        for (const char &c : data_) {
            switch (c) {
            case '"':
                type += "&quot;"sv;
                // out << "&quot;"sv;
                break;
            case '\'':
                type += "&apos;"sv;
                // out << "&apos;"sv;
                break;
            case '<':
                type += "&lt;"sv;
                // out << "&lt;"sv;
                break;
            case '>':
                type += "&gt;"sv;
                // out << "&gt;"sv;
                break;
            case '&':
                type += "&amp;"sv;
                // out << "&amp;"sv;
                break;
            default:
                type += c;
                // out << c;
                break;
            }
        }
    }
    out << type;
    out << "</text>"sv;
}
// ---------- End Text ------------------

// ---------- Document ------------------
void Document::AddPtr(std::unique_ptr<Object> &&obj) {
    objects_ptr_.emplace_back(std::move(obj));
}

void Document::Render(std::ostream &out) const {
    out << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>"sv << std::endl;
    out << "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">"sv << std::endl;
    for (const auto &object : objects_ptr_) {
        object->Render(out);
    }
    out << "</svg>"sv;
}
// ---------- End Document ------------------

std::ostream &operator<<(std::ostream &out, const svg::StrokeLineCap &line_cap) {
    switch (line_cap) {
    case StrokeLineCap::BUTT:
        out << "butt"sv;
        break;
    case StrokeLineCap::ROUND:
        out << "round"sv;
        break;
    case StrokeLineCap::SQUARE:
        out << "square"sv;
        break;
    }
    return out;
}

std::ostream &operator<<(std::ostream &out, const svg::StrokeLineJoin &line_join) {
    switch (line_join) {
    case StrokeLineJoin::ARCS:
        out << "arcs"s;
        break;
    case StrokeLineJoin::BEVEL:
        out << "bevel"s;
        break;
    case StrokeLineJoin::MITER:
        out << "miter"s;
        break;
    case StrokeLineJoin::MITER_CLIP:
        out << "miter-clip"s;
        break;
    case StrokeLineJoin::ROUND:
        out << "round"s;
        break;
    }
    return out;
}

std::ostream &operator<<(std::ostream &out, const svg::Color &col) {
    std::ostringstream strm;
    std::visit(OstreamColorPrint{strm}, col);
    out << strm.str();
    return out;
}
} // namespace svg
