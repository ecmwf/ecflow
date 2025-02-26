/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/node/Permissions.hpp"

#include "ecflow/attribute/Variable.hpp"
#include "ecflow/core/Str.hpp"

namespace ecf {

Permissions Permissions::make_from_variable(const std::string& value) {
    std::vector<std::string> allowed;
    ecf::Str::split(value, allowed, ",");
    return Permissions(std::move(allowed));
}

Permissions Permissions::find_in(const std::vector<Variable>& variables) {
    if (auto found = std::find_if(
            std::begin(variables), std::end(variables), [](auto&& var) { return var.name() == "PERMISSIONS"; });
        found != std::end(variables)) {
        auto var_value = found->theValue();
        return ecf::Permissions::make_from_variable(var_value);
    }
    else {
        return ecf::Permissions::make_empty();
    }
}

} // namespace ecf
