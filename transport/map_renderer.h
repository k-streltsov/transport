#pragma once
#include "json.h"
#include "sphere.h"
#include "descriptions.h"

class MapRenderer {
public:
    MapRenderer(const Descriptions::StopsDict& stops_dict,
                const Descriptions::BusesDict& buses_dict,
                const Json::Dict& render_settings_json);

    Svg::Document Render() const;
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
        std::vector<Svg::Color> palette;
        int bus_label_font_size = 0;
        Svg::Point bus_label_offset;
        std::vector<std::string> layers;
    };

    static RenderSettings MakeRenderSettings(const Json::Dict&);
    std::map<std::string, Svg::Point> ComputeStopsCoords(const Descriptions::StopsDict&) const;
    std::unordered_map<std::string, Svg::Color> ChooseBusColors() const;

    void RenderBusLines(Svg::Document&) const;
    void RenderBusLabels(Svg::Document&) const;
    void RenderStopPoints(Svg::Document&) const;
    void RenderStopLabels(Svg::Document&) const;

    inline static const std::unordered_map<std::string, void (MapRenderer::*)(Svg::Document&) const> layer_actions = {
        {"bus_lines",   &MapRenderer::RenderBusLines},
        {"bus_labels",  &MapRenderer::RenderBusLabels},
        {"stop_points", &MapRenderer::RenderStopPoints},
        {"stop_labels", &MapRenderer::RenderStopLabels},
    };

    const RenderSettings render_settings_;
    const Descriptions::BusesDict& buses_dict_;
    const std::map<std::string, Svg::Point> stops_coords_;
    const std::unordered_map<std::string, Svg::Color> bus_colors_;
};