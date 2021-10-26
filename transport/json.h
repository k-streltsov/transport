#pragma once

#include "svg.h"

#include <iostream>
#include <map>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace Json {

    class Node;
        using Dict = std::map<std::string, Node>;

    class Node : std::variant<std::vector<Node>, Dict, bool, int, double, std::string> {
    public:
        using variant::variant;
        const variant& GetBase() const { return *this; }

        const auto& AsArray() const { return std::get<std::vector<Node>>(*this); }
        const auto& AsMap() const { return std::get<Dict>(*this); }
        bool AsBool() const { return std::get<bool>(*this); }
        int AsInt() const { return std::get<int>(*this); }
        double AsDouble() const {
            return std::holds_alternative<double>(*this) ? std::get<double>(*this) : std::get<int>(*this);
        }
        const auto& AsString() const { return std::get<std::string>(*this); }

        Svg::Point AsPoint() const {
            const auto& point_json = AsArray();
            return {point_json[0].AsDouble(), point_json[1].AsDouble()};
        }

        Svg::Color AsColor() const {
            if (std::holds_alternative<std::string>(*this)) {
                return std::get<std::string>(*this);
            }

            const auto& rgb_json = AsArray();
            Svg::Rgb rgb = {
                static_cast<uint8_t>(rgb_json[0].AsInt()),
                static_cast<uint8_t>(rgb_json[1].AsInt()),
                static_cast<uint8_t>(rgb_json[2].AsInt())
            };

            if (rgb_json.size() == 4) {
                return Svg::Rgba{rgb, rgb_json[3].AsDouble()};
            }

            return rgb;
        }

        std::vector<Svg::Color> AsColorArray() const {
            std::vector<Svg::Color> result;
            const auto& color_array_json = AsArray();
            result.reserve(color_array_json.size());
            for (const auto& color_json : color_array_json) {
                result.push_back(color_json.AsColor());
            }
            return result;
        }
    };

    bool operator==(const Node&, const Node&);

    class Document {
    public:
        Document() = default;
        explicit Document(Node root) : root(move(root)) {}

    const Node& GetRoot() const {
        return root;
    }

    private:
        Node root;
    };

    Node LoadNode(std::istream& input);

    Document Load(std::istream& input);

    void PrintNode(const Node& node, std::ostream& output);

    template <typename Value>
    void PrintValue(const Value& value, std::ostream& output) {
        output << value;
    }

    template <>
    void PrintValue<std::string>(const std::string& value, std::ostream& output);

    template <>
    void PrintValue<bool>(const bool& value, std::ostream& output);

    template <>
    void PrintValue<std::vector<Node>>(const std::vector<Node>& nodes, std::ostream& output);

    template <>
    void PrintValue<Dict>(const Dict& dict, std::ostream& output);

    void Print(const Document& document, std::ostream& output);
}

