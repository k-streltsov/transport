#pragma once

#include "descriptions.h"
#include "json.h"
#include "transport_router.h"
#include "transport_renderer.h"
#include "utils.h"

#include <optional>
#include <set>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace Responses {
    struct Stop {
        std::set<std::string> bus_names;
    };

    struct Bus {
        size_t stop_count = 0;
        size_t unique_stop_count = 0;
        int road_route_length = 0;
        double geo_route_length = 0.0;
    };
}

class TransportCatalog {
private:
    using Bus = Responses::Bus;
    using Stop = Responses::Stop;

public:
    TransportCatalog() = default;
    TransportCatalog(std::vector<Descriptions::InputQuery> data,
                     const Json::Dict& routing_settings_json,
                     const Json::Dict& render_setting_json);

    const Stop* GetStop(const std::string& name) const;
    const Bus* GetBus(const std::string& name) const;

    std::optional<TransportRouter::RouteInfo> FindRoute(const std::string& stop_from, const std::string& stop_to) const;

    const std::string& RenderMap() const;

private:
    static int ComputeRoadRouteLength(
        const std::vector<std::string>& stops,
        const Descriptions::StopsDict& stops_dict
    );

    static double ComputeGeoRouteDistance(
        const std::vector<std::string>& stops,
        const Descriptions::StopsDict& stops_dict
    );

    std::map<std::string, Stop> stops_;
    std::map<std::string, Bus> buses_;
    std::unique_ptr<TransportRouter> router_;
    std::unique_ptr<TransportRenderer> renderer_;
    std::optional<std::string> map;
};
