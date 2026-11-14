// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include "ecflow/core/cfg/CTree.hpp"

#include <nlohmann/json.hpp>

namespace ecf::cfg {

CTree::CTree(std::string path, Node node)
    : path_{std::move(path)},
      node_{std::move(node)} {
}

const CTree* CTree::find(std::string_view key) const {
    if (const auto* mapping = std::get_if<Mapping>(&node_)) {
        for (const auto& [k, value] : *mapping) {
            if (k == key) {
                return &value;
            }
        }
    }
    return nullptr;
}

namespace {

nlohmann::ordered_json to_ordered_json(const CTree& tree) {
    const auto& node = tree.node();
    if (std::holds_alternative<CTree::Null>(node)) {
        return nullptr;
    }
    if (const auto* b = std::get_if<bool>(&node)) {
        return *b;
    }
    if (const auto* i = std::get_if<std::int64_t>(&node)) {
        return *i;
    }
    if (const auto* d = std::get_if<double>(&node)) {
        return *d;
    }
    if (const auto* s = std::get_if<std::string>(&node)) {
        return *s;
    }
    if (const auto* sequence = std::get_if<CTree::Sequence>(&node)) {
        auto array = nlohmann::ordered_json::array();
        for (const auto& item : *sequence) {
            array.push_back(to_ordered_json(item));
        }
        return array;
    }
    auto object = nlohmann::ordered_json::object();
    for (const auto& [key, value] : std::get<CTree::Mapping>(node)) {
        object[key] = to_ordered_json(value);
    }
    return object;
}

} // namespace

std::string to_compact_json(const CTree& tree) {
    return to_ordered_json(tree).dump();
}

} // namespace ecf::cfg
