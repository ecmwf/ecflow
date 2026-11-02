// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include "ecflow/core/cfg/CliReader.hpp"

namespace ecf::cfg {

CliReader::CliReader(const Catalogue& catalogue)
    : catalogue_{catalogue} {
}

Layer CliReader::read(const std::map<std::string, std::string>& options) const {
    Layer layer(layers::cli);
    for (const auto& setting : catalogue_.settings()) {
        const auto& name = setting.names().cli;
        if (!name) {
            continue;
        }
        if (const auto found = options.find(*name); found != options.end()) {
            try {
                layer.entries.emplace(setting.key(), setting.parse(found->second));
            }
            catch (const InvalidValue& e) {
                throw ConfigurationError("Invalid server command line: --" + *name + ": invalid value " + e.value() +
                                         ": " + e.reason());
            }
        }
    }
    return layer;
}

} // namespace ecf::cfg
