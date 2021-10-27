#pragma once
#include "utils.h"

#include <iostream>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>
#include <memory>
#include <string_view>

namespace Svg {

    struct Point {
        double x = 0;
        double y = 0;
    };

    struct Rgb {
        uint8_t red;
        uint8_t green;
        uint8_t blue;
    };

    struct Rgba : Rgb {
        double alpha;
    };

    using Color = std::variant<std::monostate, std::string, Rgb, Rgba>;
    const Color NoneColor{};

    void RenderColor(std::ostream&, std::monostate);
    void RenderColor(std::ostream&, const std::string&);
    void RenderColor(std::ostream&, Rgb);
    void RenderColor(std::ostream&, Rgba);
    void RenderColor(std::ostream&, const Color&);

    class Object {
    public:
        virtual void Render(std::ostream& out) const = 0;
        virtual bool EqualTo(const Object&) const = 0;
        virtual ~Object() = default;
    };

    template <typename Owner>
    class PathProps {
    public:
        Owner& SetFillColor(const Color& color);
        Owner& SetStrokeColor(const Color& color);
        Owner& SetStrokeWidth(double value);
        Owner& SetStrokeLineCap(std::string_view);
        Owner& SetStrokeLineJoin(std::string_view);
        void RenderAttrs(std::ostream& out) const;
        void SetProp(std::string_view, std::string_view);
    protected:
        bool EqualProps(const PathProps<Owner>&) const;
    private:
        Color fill_color_;
        Color stroke_color_;
        double stroke_width_ = 1.0;
        std::optional<std::string> stroke_line_cap_;
        std::optional<std::string> stroke_line_join_;
    
    private:
        Owner& AsOwner();
    };

    class Circle : public Object, public PathProps<Circle> {
    public:
        friend bool operator==(const Circle&, const Circle&);

        Circle() = default;
        Circle(std::string_view);

        Circle& SetCenter(Point point);
        Circle& SetRadius(double radius);
        void Render(std::ostream& out) const override;
        bool EqualTo(const Object&) const override;
    private:
        Point center_;
        double radius_ = 1;
    };

    class Polyline : public Object, public PathProps<Polyline> {
    public:
        friend bool operator==(const Polyline&, const Polyline&);

        Polyline() = default;
        Polyline(std::string_view);

        Polyline& AddPoint(Point point);
        void Render(std::ostream& out) const override;
        bool EqualTo(const Object&) const override;
    private:
        std::vector<Point> points_;
    };

    class Text : public Object, public PathProps<Text> {
    public:
        friend bool operator==(const Text&, const Text&);

        Text() = default;
        Text(std::string_view, std::string_view);
        Text& SetPoint(Point point);
        Text& SetOffset(Point point);
        Text& SetFontSize(uint32_t size);
        Text& SetFontFamily(const std::string& value);
        Text& SetData(const std::string& data);
        Text& SetFontWeight(const std::string&);
        void Render(std::ostream& out) const override;
        bool EqualTo(const Object&) const override;
    private:
        Point point_;
        Point offset_;
        uint32_t font_size_ = 1;
        std::optional<std::string> font_family_;
        std::string data_;
        std::optional<std::string> font_weight_;
    };

    Point ParsePoint(std::string_view);
    std::vector<Point> ParsePoints(std::string_view);
    double ParseDouble(std::string_view);
    Color ParseColor(std::string_view);

    class Document {
    public:
        friend bool operator==(const Document&, const Document&);

        Document() = default;
        Document(std::string_view);
        template <typename ObjectType>
        void Add(ObjectType object);

        void Render(std::ostream& out) const;
    private:
        std::vector<std::unique_ptr<Object>> objects_;
    };

    template <typename Owner>
    Owner& PathProps<Owner>::AsOwner() {
        return static_cast<Owner&>(*this);
    }

    template <typename Owner>
    Owner& PathProps<Owner>::SetFillColor(const Color& color) {
        fill_color_ = color;
        return AsOwner();
    }

    template <typename Owner>
    Owner& PathProps<Owner>::SetStrokeColor(const Color& color) {
        stroke_color_ = color;
        return AsOwner();
    }

    template <typename Owner>
    Owner& PathProps<Owner>::SetStrokeWidth(double value) {
        stroke_width_ = value;
        return AsOwner();
    }

    template <typename Owner>
    Owner& PathProps<Owner>::SetStrokeLineCap(std::string_view value) {
        stroke_line_cap_ = value;
        return AsOwner();
    }

    template <typename Owner>
    Owner& PathProps<Owner>::SetStrokeLineJoin(std::string_view value) {
        stroke_line_join_ = value;
        return AsOwner();
    }

    template <typename Owner>
    void PathProps<Owner>::RenderAttrs(std::ostream& out) const {
        out << "fill=\"";
        RenderColor(out, fill_color_);
        out << "\" ";
        out << "stroke=\"";
        RenderColor(out, stroke_color_);
        out << "\" ";
        out << "stroke-width=\"" << stroke_width_ << "\" ";
        if (stroke_line_cap_) {
            out << "stroke-linecap=\"" << *stroke_line_cap_ << "\" ";
        }
        if (stroke_line_join_) {
            out << "stroke-linejoin=\"" << *stroke_line_join_ << "\" ";
        }
    }

    template <typename Owner>
    void PathProps<Owner>::SetProp(std::string_view name, std::string_view value) {
        if (name == "fill") {
            SetFillColor(ParseColor(value));
        } else if (name == "stroke") {
            SetStrokeColor(ParseColor(value));
        } else if (name == "stroke-width") {
            SetStrokeWidth(ParseDouble(value));
        } else if (name == "stroke-linecap") {
            SetStrokeLineCap(value);
        } else if (name == "stroke-linejoin") {
            SetStrokeLineJoin(value);
        }
    }

    template <typename Owner>
    bool PathProps<Owner>::EqualProps(const PathProps<Owner>& other) const {
        if (!(fill_color_ == other.fill_color_)) {
            return false;
        }

        if (!(stroke_color_ == other.stroke_color_)) {
            return false;
        }

        if (!EqualWithAccuracy(stroke_width_, other.stroke_width_)) {
            return false;
        }

        if (stroke_line_cap_ != other.stroke_line_cap_) {
            return false;
        }

        if (stroke_line_join_ != other.stroke_line_join_) {
            return false;
        }

        return true;
    }

    template <typename ObjectType>
    void Document::Add(ObjectType object) {
        objects_.push_back(std::make_unique<ObjectType>(std::move(object)));
    }

    bool operator==(Point lhs, Point rhs);
    bool operator==(Rgb lhs, Rgb rhs);
    bool operator==(Rgba lhs, Rgba rhs);
    bool operator==(const Polyline&, const Polyline&);
    bool operator==(const Circle&, const Circle&);
    bool operator==(const Text&, const Text&);
    bool operator==(const Document&, const Document&);
    std::ostream& operator<<(std::ostream&, const Polyline&);
    std::ostream& operator<<(std::ostream&, const Circle&);
    std::ostream& operator<<(std::ostream&, const Text&);
    std::ostream& operator<<(std::ostream&, const Document&);
    /*std::ostream& operator<<(std::ostream&, const Rgb&);
    std::ostream& operator<<(std::ostream&, const Rgba&);*/
}