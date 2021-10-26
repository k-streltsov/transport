#pragma once
#include "json.h"
#include "sphere.h"
#include "descriptions.h"

class TransportRenderer {
public:
    TransportRenderer(const Descriptions::StopsDict& stops_dict,
                      const Descriptions::BusesDict& buses_dict,
                      const Json::Dict& render_settings_json,
                      const Sphere::Point min_coords,
                      const Sphere::Point max_coords);

    const std::string& RenderMap() const;
private:
    struct RenderSettings {
        double width = 0.0;
        double height = 0.0;
        double padding = 0.0;
        double stop_radius = 0.0;
        double line_width = 0.0;
        int stop_label_font_size = 0;
        Svg::Point stop_label_offset;
        Svg::Color underlayer_color;
        double underlayer_width = 0.0;
        std::vector<Svg::Color> color_palette;
        int bus_label_font_size = 0;
        Svg::Point bus_label_offset;
    };

    static RenderSettings MakeRenderSettings(const Json::Dict&);
    double CalculateZoomCoeff() const;
    Svg::Point ProjectToSvgCoords(double, Sphere::Point) const;

    using StopsSvgCoords = std::map<std::string, Svg::Point>;
    std::string RenderMap(const StopsSvgCoords&, const Descriptions::BusesDict&) const;
    void RenderBusesRoutes(Svg::Document&, const StopsSvgCoords&, const Descriptions::BusesDict&) const;
    void RenderBusesTitles(Svg::Document&, const StopsSvgCoords&, const Descriptions::BusesDict&) const;
    void RenderStops(Svg::Document&, const StopsSvgCoords&) const;
    void RenderStopsTitles(Svg::Document&, const StopsSvgCoords&) const;

    const RenderSettings render_settings_;
    const Sphere::Point min_coords_;
    const Sphere::Point max_coords_;
    std::string map_;
};