#include "descriptions.h"

using namespace std;

namespace Descriptions {

    Stop Stop::ParseFrom(const Json::Dict& attrs) {
        Stop stop = {
            .name = attrs.at("name").AsString(),
            .position = {
                .latitude = attrs.at("latitude").AsDouble(),
                .longitude = attrs.at("longitude").AsDouble(),
            }
        };
        if (attrs.count("road_distances") > 0) {
            for (const auto& [neighbour_stop, distance_node] : attrs.at("road_distances").AsMap()) {
                stop.distances[neighbour_stop] = distance_node.AsInt();
            }
        }
        return stop;
    }

    int ComputeStopsDistance(const Stop& lhs, const Stop& rhs) {
        if (auto it = lhs.distances.find(rhs.name); it != lhs.distances.end()) {
            return it->second;
        } else {
            return rhs.distances.at(lhs.name);
        }
    }

    vector<string> ParseStops(const vector<Json::Node>& stop_nodes, bool is_roundtrip) {
        vector<string> stops;
        stops.reserve(stop_nodes.size());
        for (const Json::Node& stop_node : stop_nodes) {
            stops.push_back(stop_node.AsString());
        }
        if (is_roundtrip || stops.size() <= 1) {
            return stops;
        }
        stops.reserve(stops.size() * 2 - 1);  // end stop is not repeated
        for (size_t stop_idx = stops.size() - 1; stop_idx > 0; --stop_idx) {
            stops.push_back(stops[stop_idx - 1]);
        }
        return stops;
    }

    Bus Bus::ParseFrom(const Json::Dict& attrs) {       
        const auto& name = attrs.at("name").AsString();
        const auto& stops = attrs.at("stops").AsArray();

        if (stops.empty()) {
            return Bus{.name = name};
        }

        const auto& begin_stop = stops.front().AsString();
        const auto& end_stop = stops.back().AsString();
        Bus bus{
            .name = name,
            .stops = ParseStops(stops, attrs.at("is_roundtrip").AsBool()),
            .endpoints = {stops.front().AsString(), stops.back().AsString()}
        };

        if (bus.endpoints.front() == bus.endpoints.back()) {
            bus.endpoints.pop_back();
        }
        return bus;
    }

    std::vector<InputQuery> ReadDescriptions(const vector<Json::Node>& nodes) {
        
        vector<InputQuery> queries;
        queries.reserve(nodes.size());

        for (const Json::Node& node : nodes) {
            const auto& node_dict = node.AsMap();
            if (node_dict.at("type").AsString() == "Bus") {
                queries.push_back(Bus::ParseFrom(node_dict));
            } else {
                auto stop = Stop::ParseFrom(node_dict);

                queries.push_back(move(stop));
            }
        }

        return queries;
    }

}
