#include "map_renderer.h"
#include "sphere_projection.h"

#include <algorithm>
using namespace std;
using namespace Descriptions;

Svg::Point ParsePoint(const Json::Node& json) {
    const auto& array = json.AsArray();
    return {
        array[0].AsDouble(),
        array[1].AsDouble()
    };
}

Svg::Color ParseColor(const Json::Node& json) {
    if (json.IsString()) {
        return json.AsString();
    }
    const auto& array = json.AsArray();
    Svg::Rgb rgb{
        static_cast<uint8_t>(array[0].AsInt()),
        static_cast<uint8_t>(array[1].AsInt()),
        static_cast<uint8_t>(array[2].AsInt())
    };
    if (array.size() == 3) {
        return rgb;
    } else {
        return Svg::Rgba{rgb, array[3].AsDouble()};
    }
}

vector<Svg::Color> ParseColors(const Json::Node& json) {
    const auto& array = json.AsArray();
    vector<Svg::Color> colors;
    colors.reserve(array.size());
    transform(begin(array), end(array), back_inserter(colors), ParseColor);
    return colors;
}


MapRenderer::MapRenderer(const StopsDict& stops_dict,
                         const BusesDict& buses_dict,
                         const Json::Dict& render_settings_json)
    : render_settings_(MakeRenderSettings(render_settings_json)),
      buses_dict_(buses_dict),
      stops_coords_(ComputeStopsCoords(stops_dict)),
      bus_colors_(ChooseBusColors())
{}

unordered_map<string, Svg::Color> MapRenderer::ChooseBusColors() const {
    const auto& palette = render_settings_.palette;
    unordered_map<std::string, Svg::Color> bus_colors;
    int idx = 0;
    for (const auto& [bus_name, bus_ptr] : buses_dict_) {
        bus_colors[bus_name] = palette[idx++ % palette.size()];
    }
    return bus_colors;
}

map<string, Svg::Point> MapRenderer::ComputeStopsCoords(const StopsDict& stops_dict) const {
    vector<Sphere::Point> points;
    points.reserve(stops_dict.size());
    for (const auto& [_, stop_ptr] : stops_dict) {
        points.push_back(stop_ptr->position);
    }

    const double max_width = render_settings_.width;
    const double max_height = render_settings_.height;
    const double padding = render_settings_.padding;

    const Sphere::Projector projector(
        begin(points), end(points),
        max_width, max_height, padding
    );
    
    map<string, Svg::Point> stops_coords;
    for (const auto& [stop_name, stop_ptr] : stops_dict) {
        stops_coords[stop_name] = projector(stop_ptr->position);
    }

    return stops_coords;
}

MapRenderer::RenderSettings MapRenderer::MakeRenderSettings(const Json::Dict& json) {
    RenderSettings render_settings = {
        .width = json.at("width").AsDouble(),
        .height = json.at("height").AsDouble(),
        .padding = json.at("padding").AsDouble(),
        .stop_radius = json.at("stop_radius").AsDouble(),
        .line_width = json.at("line_width").AsDouble(),
        .stop_label_font_size = json.at("stop_label_font_size").AsInt(),
        .stop_label_offset = ParsePoint(json.at("stop_label_offset")),
        .underlayer_color = ParseColor(json.at("underlayer_color")),
        .underlayer_width = json.at("underlayer_width").AsDouble(),
        .palette = ParseColors(json.at("color_palette")),
        .bus_label_font_size = json.at("bus_label_font_size").AsInt(),
        .bus_label_offset = ParsePoint(json.at("bus_label_offset"))
    };

    const auto& layers_array = json.at("layers").AsArray();
    render_settings.layers.reserve(layers_array.size());
    for (const auto& layer_node : layers_array) {
        render_settings.layers.push_back(layer_node.AsString());
    }

    return render_settings;
}

void MapRenderer::RenderBusLines(Svg::Document& svg) const {
    for (const auto& [bus_name, bus_ptr] : buses_dict_) {
        const auto& stops = bus_ptr->stops;
        if (stops.empty()) {
            continue;
        }

        Svg::Polyline line;
        line.SetStrokeColor(bus_colors_.at(bus_name))
            .SetStrokeWidth(render_settings_.line_width)
            .SetStrokeLineCap("round")
            .SetStrokeLineJoin("round");

        for (const auto& stop : stops) {
            line.AddPoint(stops_coords_.at(stop));
        }

        svg.Add(line);
    }
}

void MapRenderer::RenderStopPoints(Svg::Document& svg) const {
    for (const auto& [stop_name, stop_point] : stops_coords_) {
        svg.Add(
            Svg::Circle{}
            .SetCenter(stop_point)
            .SetRadius(render_settings_.stop_radius)
            .SetFillColor("white")
        );
    }
}

void MapRenderer::RenderStopLabels(Svg::Document& svg) const {
    for (const auto& [stop_name, stop_point] : stops_coords_) {
        const auto label =
            Svg::Text{}
            .SetPoint(stop_point)
            .SetOffset(render_settings_.stop_label_offset)
            .SetFontSize(render_settings_.stop_label_font_size)
            .SetFontFamily("Verdana")
            .SetData(stop_name);

        svg.Add(
            Svg::Text{label}
            .SetFillColor(render_settings_.underlayer_color)
            .SetStrokeColor(render_settings_.underlayer_color)
            .SetStrokeWidth(render_settings_.underlayer_width)
            .SetStrokeLineCap("round")
            .SetStrokeLineJoin("round")
        );
        svg.Add(
            Svg::Text(label)
            .SetFillColor("black")
        );
    }
}

void MapRenderer::RenderBusLabels(Svg::Document& svg) const {
    for (const auto& [bus_name, bus_ptr] : buses_dict_) {
        const auto& stops = bus_ptr->stops;
        const auto& color = bus_colors_.at(bus_name);
        for (const string& endpoint : bus_ptr->endpoints) {
            const auto point = stops_coords_.at(endpoint);
            const auto base_text =
                Svg::Text{}
                .SetPoint(point)
                .SetOffset(render_settings_.bus_label_offset)
                .SetFontSize(render_settings_.bus_label_font_size)
                .SetFontFamily("Verdana")
                .SetFontWeight("bold")
                .SetData(bus_name);
            svg.Add(
                Svg::Text(base_text)
                .SetFillColor(render_settings_.underlayer_color)
                .SetStrokeColor(render_settings_.underlayer_color)
                .SetStrokeWidth(render_settings_.underlayer_width)
                .SetStrokeLineCap("round").SetStrokeLineJoin("round")
            );
            svg.Add(
                Svg::Text(base_text)
                .SetFillColor(color)
            );
        }
    }
}

Svg::Document MapRenderer::Render() const {
    Svg::Document svg;
    for (const auto& layer : render_settings_.layers) {
        (this->*layer_actions.at(layer))(svg);
        /*const auto& layer = layer_node.AsString();
        if (layer == "bus_lines") {
            RenderBusLines(svg);
        } else if (layer == "bus_labels") {
            RenderBusLabels(svg);
        } else if (layer == "stop_points") {
            RenderStopPoints(svg);
        } else if (layer == "stop_labels") {
            RenderStopLabels(svg);
        }*/
    }

    return svg;
}