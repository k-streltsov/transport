#include "transport_renderer.h"

#include <sstream>
using namespace std;
using namespace Descriptions;
using namespace Svg;

TransportRenderer::TransportRenderer(const StopsDict& stops_dict,
                                     const BusesDict& buses_dict,
                                     const Json::Dict& render_settings_json,
                                     const Sphere::Point min_coords,
                                     const Sphere::Point max_coords)
    : render_settings_(MakeRenderSettings(render_settings_json))
    , min_coords_(min_coords)
    , max_coords_(max_coords) {

    double zoom_coeff = CalculateZoomCoeff();
    map<string, Svg::Point> stops_svg_coords;
    for (const auto& [stop, data] : stops_dict) {
        stops_svg_coords.insert({stop, ProjectToSvgCoords(zoom_coeff, data->position)});
    }

    map_ = RenderMap(stops_svg_coords, buses_dict);
}

TransportRenderer::RenderSettings TransportRenderer::MakeRenderSettings(const Json::Dict& json) {
    return RenderSettings{
        .width = json.at("width").AsDouble(),
        .height = json.at("height").AsDouble(),
        .padding = json.at("padding").AsDouble(),
        .stop_radius = json.at("stop_radius").AsDouble(),
        .line_width = json.at("line_width").AsDouble(),
        .stop_label_font_size = json.at("stop_label_font_size").AsInt(),
        .stop_label_offset = json.at("stop_label_offset").AsPoint(),
        .underlayer_color = json.at("underlayer_color").AsColor(),
        .underlayer_width = json.at("underlayer_width").AsDouble(),
        .color_palette = json.at("color_palette").AsColorArray(),
        .bus_label_font_size = json.at("bus_label_font_size").AsInt(),
        .bus_label_offset = json.at("bus_label_offset").AsPoint()
    };
}

double TransportRenderer::CalculateZoomCoeff() const {
    double w = max_coords_.longitude - min_coords_.longitude;
    double width_zoom_coef = w != 0 ? (render_settings_.width - 2 * render_settings_.padding) / w : 0;

    double h = max_coords_.latitude - min_coords_.latitude;
    double height_zoom_coef = h != 0 ? (render_settings_.height - 2 * render_settings_.padding) / h : 0;
    if (width_zoom_coef != 0 && height_zoom_coef != 0) {
        return std::min(width_zoom_coef, height_zoom_coef);
    } else if (width_zoom_coef == 0) {
        return height_zoom_coef;
    } else {
        return width_zoom_coef;
    }
}

Svg::Point TransportRenderer::ProjectToSvgCoords(double zoom_coeff, Sphere::Point coords) const {
    return {
        (coords.longitude - min_coords_.longitude) * zoom_coeff + render_settings_.padding,
        (max_coords_.latitude - coords.latitude) * zoom_coeff + render_settings_.padding
    };
}

void TransportRenderer::RenderBusesRoutes(Document& doc,
                                          const StopsSvgCoords& stops_svg_coords,
                                          const BusesDict& buses_dict) const {
    size_t color_idx = 0;
    for (const auto& [bus, data] : buses_dict) {
        Polyline polyline;
        for (const auto& stop : data->stops) {
            polyline.AddPoint(stops_svg_coords.at(stop));
        }

        if (color_idx >= render_settings_.color_palette.size()) {
            color_idx = 0;
        }
        doc.Add(
            polyline
            .SetStrokeColor(render_settings_.color_palette[color_idx++])
            .SetStrokeWidth(render_settings_.line_width)
            .SetStrokeLineCap("round")
            .SetStrokeLineJoin("round")
        );
    }
}

void TransportRenderer::RenderBusesTitles(Document& doc,
                                          const StopsSvgCoords& stops_svg_coords,
                                          const BusesDict& buses_dict) const {
    size_t color_idx = 0;
    for (const auto& [bus, data] : buses_dict) {
        const auto& stops = data->stops;
        Text underlayer;
        Text title =
            underlayer
            .SetPoint(stops_svg_coords.at(stops.front()))
            .SetOffset(render_settings_.bus_label_offset)
            .SetFontSize(render_settings_.bus_label_font_size)
            .SetFontFamily("Verdana")
            .SetFontWeight("bold")
            .SetData(bus);

        underlayer
            .SetFillColor(render_settings_.underlayer_color)
            .SetStrokeColor(render_settings_.underlayer_color)
            .SetStrokeWidth(render_settings_.underlayer_width)
            .SetStrokeLineCap("round")
            .SetStrokeLineJoin("round");

        if (color_idx >= render_settings_.color_palette.size()) {
            color_idx = 0;
        }
        title
            .SetFillColor(render_settings_.color_palette[color_idx++]);

        doc.Add(underlayer);
        doc.Add(title);
        if (stops.front() != stops.back()) {
            const auto coords = stops_svg_coords.at(stops.front());
            doc.Add(underlayer.SetPoint(coords));
            doc.Add(title.SetPoint(coords));
        }
    }
}

void TransportRenderer::RenderStops(Document& doc, const StopsSvgCoords& stops_svg_coords) const {
    for (const auto& [stop, coords] : stops_svg_coords) {
        doc.Add(
            Circle{}
            .SetCenter(coords)
            .SetRadius(render_settings_.stop_radius)
            .SetFillColor("white")
        );
    }
}

void TransportRenderer::RenderStopsTitles(Document& doc, const StopsSvgCoords& stops_svg_coords) const {
    for (const auto& [stop, coords] : stops_svg_coords) {
        Text underlayer;
        Text title =
            underlayer
            .SetPoint(coords)
            .SetOffset(render_settings_.stop_label_offset)
            .SetFontSize(render_settings_.stop_label_font_size)
            .SetFontFamily("Verdana")
            .SetData(stop);

        underlayer
            .SetFillColor(render_settings_.underlayer_color)
            .SetStrokeColor(render_settings_.underlayer_color)
            .SetStrokeWidth(render_settings_.underlayer_width)
            .SetStrokeLineCap("round")
            .SetStrokeLineJoin("round");

        title.SetFillColor("black");
        doc.Add(underlayer);
        doc.Add(title);
    }
}

string TransportRenderer::RenderMap(const StopsSvgCoords& stops_svg_coords,
                                    const BusesDict& buses_dict) const {
    Document doc;
    RenderBusesRoutes(doc, stops_svg_coords, buses_dict);
    RenderBusesTitles(doc, stops_svg_coords, buses_dict);
    RenderStops(doc, stops_svg_coords);
    RenderStopsTitles(doc, stops_svg_coords);

    ostringstream oss;
    doc.Render(oss);
    return oss.str();
}

const string& TransportRenderer::RenderMap() const {
    return map_;
}