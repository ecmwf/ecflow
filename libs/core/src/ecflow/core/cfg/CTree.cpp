// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include "ecflow/core/cfg/CTree.hpp"

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

} // namespace ecf::cfg
