/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ecflow/node/Permissions.hpp"

#include "ecflow/attribute/Variable.hpp"
#include "ecflow/core/Str.hpp"

namespace ecf {

Permissions Permissions::make_from_variable(const std::string& value) {
    std::vector<std::string> allowed;
    ecf::algorithm::split_at(allowed, value, ",");
    return Permissions(std::move(allowed));
}

Permissions Permissions::find_in(const std::vector<Variable>& variables) {
    if (auto found = std::find_if(
            std::begin(variables), std::end(variables), [](auto&& var) { return var.name() == "PERMISSIONS"; });
        found != std::end(variables)) {
        auto var_value = found->value();
        return ecf::Permissions::make_from_variable(var_value);
    }
    else {
        return ecf::Permissions::make_empty();
    }
}

} // namespace ecf
