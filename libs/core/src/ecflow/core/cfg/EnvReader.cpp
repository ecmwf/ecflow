// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include "ecflow/core/cfg/EnvReader.hpp"

#include <string>

#include "ecflow/core/Environment.hpp"

namespace ecf::cfg {

EnvReader::EnvReader(const Catalogue& catalogue)
    : catalogue_{catalogue} {
}

Layer EnvReader::read() const {
    Layer layer(layers::env);
    for (const auto& setting : catalogue_.settings()) {
        const auto& name = setting.names().env;
        if (!name) {
            continue;
        }
        if (const auto text = ecf::environment::fetch<std::string>(name->c_str())) {
            try {
                layer.entries.emplace(setting.key(), setting.parse(*text));
            }
            catch (const InvalidValue& e) {
                throw ConfigurationError("Environment variable " + *name + ": invalid value " + e.value() + ": " +
                                         e.reason());
            }
        }
    }
    return layer;
}

} // namespace ecf::cfg
