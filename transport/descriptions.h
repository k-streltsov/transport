#pragma once

#include "json.h"
#include "sphere.h"

#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace Descriptions {

    struct Stop {
        std::string name;
        Sphere::Point position;
        std::unordered_map<std::string, int> distances;

        static Stop ParseFrom(const Json::Dict& attrs);
    };

    int ComputeStopsDistance(const Stop& lhs, const Stop& rhs);

    struct Bus {
        std::string name;
        std::vector<std::string> stops;
        std::vector<std::string> endpoints;

        static Bus ParseFrom(const Json::Dict& attrs);
    };

    using InputQuery = std::variant<Stop, Bus>;

    std::vector<InputQuery> ReadDescriptions(const std::vector<Json::Node>& nodes);

    using StopsDict = std::unordered_map<std::string, const Stop*>;
    using BusesDict = std::map<std::string, const Bus*>;
}
