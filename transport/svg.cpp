#include "svg.h"
using namespace std;

namespace Svg {
    string_view ReadToken(string_view& str, string_view delimeter) {
        size_t pos = str.find(delimeter);
        auto token = Strip(str.substr(0, pos));
        str.remove_prefix(pos == string_view::npos ? str.size() : pos + delimeter.size());
        return token;
    }

    string_view ReadValue(string_view& str) {
        size_t pos = str.find('"');
        str.remove_prefix(pos + 1);
        return ReadToken(str, "\"");
    }

    Polyline::Polyline(string_view props) {
        props = Strip(props);
        while (!props.empty()) {
            auto attr = ReadToken(props, "=");
            auto value = ReadValue(props);
            if (attr == "points") {
                for (auto p : ParsePoints(value)) {
                    points_.push_back(p);
                }
            } else {
                SetProp(attr, value);
            }
        }
    }

    Circle::Circle(string_view props) {
        props = Strip(props);
        while (!props.empty()) {
            auto attr = ReadToken(props, "=");
            auto value = ReadValue(props);
            if (attr == "cx") {
                center_.x = ParseDouble(value);
            } else if (attr == "cy") {
                center_.y = ParseDouble(value);
            } else if (attr == "r") {
                radius_ = ParseDouble(value);
            } else {
                SetProp(attr, value);
            }
        }
    }

    Text::Text(string_view props, string_view data) {
        props = Strip(props);
        data_ = data;
        while (!props.empty()) {
            auto attr = ReadToken(props, "=");
            auto value = ReadValue(props);
            if (attr == "x") {
                point_.x = ParseDouble(value);
            } else if (attr == "y") {
                point_.y = ParseDouble(value);
            } else if (attr == "dx") {
                offset_.x = ParseDouble(value);
            } else if (attr == "dy") {
                offset_.y = ParseDouble(value);
            } else if (attr == "font-size") {
                font_size_ = static_cast<uint32_t>(ParseDouble(value));
            } else if (attr == "font-family") {
                font_family_ = value;
            } else {
                SetProp(attr, value);
            }
        }
    }

    Document::Document(string_view svg) {
        ReadToken(svg, ">");
        ReadToken(svg, ">");
        while (!svg.empty()) {
            auto type = ReadToken(svg, " ");
            if (type == "<polyline") {
                objects_.push_back(make_unique<Polyline>(ReadToken(svg, "/>")));
            } else if (type == "<circle") {
                objects_.push_back(make_unique<Circle>(ReadToken(svg, "/>")));
            } else if (type == "<text") {
                auto props = ReadToken(svg, ">");
                objects_.push_back(make_unique<Text>(props, ReadToken(svg, "</text>")));
            }
        }  
    }

    double ParseDouble(string_view str) {
        return stod(string(str));
    }

    Point ParsePoint(string_view str) {
        double x = ParseDouble(ReadToken(str, ","));
        double y = ParseDouble(str);
        return {x, y};
    }

    vector<Point> ParsePoints(string_view str) {
        str = Strip(str);
        vector<Point> result;       
        while (!str.empty()) {
            result.push_back(ParsePoint(ReadToken(str, " ")));
        }
        return result;
    }

    variant<Rgb, Rgba> ParseRgb(string_view str) {
        auto red = ReadToken(str, ",");
        auto green = ReadToken(str, ",");
        string blue;
        for (char c : str) {
            str.remove_prefix(1);
            if (!isdigit(c)) {
                break;
            } else {
                blue.push_back(c);
            }
        }
        Rgb rgb = {0, 0, 0};
        rgb.red = static_cast<uint8_t>(stoi(string(red)));
        rgb.green = static_cast<uint8_t>(stoi(string(green)));
        rgb.blue = static_cast<uint8_t>(stoi(blue));      
        if (str.empty()) {
            return rgb;
        }

        Rgba rgba = {rgb, 0.0};
        rgba.alpha = stod(string(str));
        return rgba;
    }

    Color ParseColor(string_view str) {
        if (size_t pos = str.find('('); pos != string_view::npos) {
            str.remove_prefix(pos + 1);
            if (auto rgb = ParseRgb(ReadToken(str, ")")); std::holds_alternative<Rgb>(rgb)) {
                return std::get<Rgb>(rgb);
            } else {
                return std::get<Rgba>(rgb);
            }
        } else if (str == "none") {
            return NoneColor;
        } else {
            return string(str);
        }
    }

    void RenderColor(ostream& out, monostate) {
        out << "none";
    }

    void RenderColor(ostream& out, const string& value) {
        out << value;
    }

    void RenderColor(ostream& out, Rgb rgb) {
        out << "rgb(" << static_cast<int>(rgb.red)
            << "," << static_cast<int>(rgb.green)
            << "," << static_cast<int>(rgb.blue) << ")";
    }

    void RenderColor(ostream& out, Rgba rgba) {
        out << "rgba(" << static_cast<int>(rgba.red)
            << "," << static_cast<int>(rgba.green)
            << "," << static_cast<int>(rgba.blue)
            << "," << rgba.alpha << ")";
    }

    void RenderColor(ostream& out, const Color& color) {
        visit([&out](const auto& value) { RenderColor(out, value); },
            color);
    }

    Circle& Circle::SetCenter(Point point) {
        center_ = point;
        return *this;
    }
    Circle& Circle::SetRadius(double radius) {
        radius_ = radius;
        return *this;
    }

    void Circle::Render(ostream& out) const {
        out << "<circle ";
        out << "cx=\"" << center_.x << "\" ";
        out << "cy=\"" << center_.y << "\" ";
        out << "r=\"" << radius_ << "\" ";
        PathProps::RenderAttrs(out);
        out << "/>";
    }

    Polyline& Polyline::AddPoint(Point point) {
        points_.push_back(point);
        return *this;
    }

    void Polyline::Render(ostream& out) const {
        out << "<polyline ";
        out << "points=\"";
        bool first = true;
        for (const Point point : points_) {
            if (first) {
                first = false;
            } else {
                out << " ";
            }
            out << point.x << "," << point.y;
        }
        out << "\" ";
        PathProps::RenderAttrs(out);
        out << "/>";
    }

    Text& Text::SetPoint(Point point) {
        point_ = point;
        return *this;
    }

    Text& Text::SetOffset(Point point) {
        offset_ = point;
        return *this;
    }

    Text& Text::SetFontSize(uint32_t size) {
        font_size_ = size;
        return *this;
    }

    Text& Text::SetFontFamily(const string& value) {
        font_family_ = value;
        return *this;
    }

    Text& Text::SetData(const string& data) {
        data_ = data;
        return *this;
    }

    Text& Text::SetFontWeight(const string& font_weight) {
        font_weight_ = font_weight;
        return *this;
    }

    void Text::Render(ostream& out) const {
        out << "<text ";
        out << "x=\"" << point_.x << "\" ";
        out << "y=\"" << point_.y << "\" ";
        out << "dx=\"" << offset_.x << "\" ";
        out << "dy=\"" << offset_.y << "\" ";
        out << "font-size=\"" << font_size_ << "\" ";
        if (font_family_) {
            out << "font-family=\"" << *font_family_ << "\" ";
        }
        if (font_weight_) {
            out << "font-weight=\"" << *font_weight_ << "\" ";
        }
        PathProps::RenderAttrs(out);
        out << ">";
        out << data_;
        out << "</text>";
    }

    void Document::Render(ostream& out) const {
        out << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>";
        out << "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">";
        for (const auto& object_ptr : objects_) {
            object_ptr->Render(out);
        }
        out << "</svg>";
    }

    bool operator==(Point lhs, Point rhs) {
        return EqualWithAccuracy(lhs.x, rhs.x) && EqualWithAccuracy(lhs.y, rhs.y);
    }

    bool operator==(Rgb lhs, Rgb rhs) {
        return lhs.red == rhs.red && lhs.green == rhs.green && lhs.blue == rhs.blue;
    }

    bool operator==(Rgba lhs, Rgba rhs) {
        if (abs(lhs.alpha - rhs.alpha) >= 1e-6) {
            return false;
        }
        return static_cast<Rgb>(lhs) == static_cast<Rgb>(rhs);
    }

    bool operator==(const Polyline& lhs, const Polyline& rhs) {
        return lhs.EqualProps(rhs) && lhs.points_ == rhs.points_;
    }

    bool operator==(const Circle& lhs, const Circle& rhs) {
        return lhs.EqualProps(rhs) && lhs.center_ == rhs.center_ && EqualWithAccuracy(lhs.radius_, rhs.radius_);
    }

    bool operator==(const Text& lhs, const Text& rhs) {
        return lhs.EqualProps(rhs)
            && lhs.point_ == rhs.point_ && lhs.offset_ == rhs.offset_
            && lhs.font_size_ == rhs.font_size_ && lhs.font_family_ == rhs.font_family_
            && lhs.data_ == rhs.data_;
    }

    bool operator==(const Document& lhs, const Document& rhs) {
        if (lhs.objects_.size() != rhs.objects_.size()) {
            return false;
        }

        for (size_t i = 0; i < lhs.objects_.size(); ++i) {
            if (!lhs.objects_[i]->EqualTo(*rhs.objects_[i])) {
                return false;
            }
        }

        return true;
    }

    std::ostream& operator<<(std::ostream& out, const Polyline& obj) {
        obj.Render(out);
        return out;
    }

    std::ostream& operator<<(std::ostream& out, const Circle& obj) {
        obj.Render(out);
        return out;
    }

    std::ostream& operator<<(ostream& out, const Text& text) {
        text.Render(out);
        return out;
    }

    std::ostream& operator<<(std::ostream& out, const Document& doc) {
        doc.Render(out);
        return out;
    }

    bool Polyline::EqualTo(const Object& other) const {
        if (const Polyline* p = dynamic_cast<const Polyline*>(&other)) {
            return *this == *p;
        } else {
            return false;
        }
    }

    bool Circle::EqualTo(const Object& other) const {
        if (const Circle* p = dynamic_cast<const Circle*>(&other)) {
            return *this == *p;
        } else {
            return false;
        }
    }

    bool Text::EqualTo(const Object& other) const {
        if (const Text* p = dynamic_cast<const Text*>(&other)) {
            return *this == *p;
        } else {
            return false;
        }
    }
}