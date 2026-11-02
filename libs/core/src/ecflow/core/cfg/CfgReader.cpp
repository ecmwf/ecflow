// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include "ecflow/core/cfg/CfgReader.hpp"

#include <fstream>
#include <map>
#include <stdexcept>
#include <string>
#include <system_error>

#include <boost/program_options.hpp>

namespace ecf::cfg {

namespace {

namespace fs = std::filesystem;
namespace po = boost::program_options;

} // namespace

CfgReader::CfgReader(const Catalogue& catalogue)
    : catalogue_{catalogue} {
}

Layer CfgReader::read(const fs::path& file) const {
    const auto prefix = "Could not parse server configuration file '" + file.string() + "': ";
    try {
        std::error_code status_error;
        const auto status = fs::status(file, status_error);
        if (status_error && status_error != std::errc::no_such_file_or_directory) {
            throw std::runtime_error("Cannot inspect configuration file: " + status_error.message());
        }
        if (!fs::exists(status)) {
            return Layer(layers::env_cfg);
        }
        if (!fs::is_regular_file(status)) {
            throw std::runtime_error("Configuration path is not a regular file");
        }
        std::ifstream in(file);
        if (!in) {
            throw std::runtime_error("Cannot open configuration file for reading");
        }

        // Every setting named in the file is an option; any other name is refused
        po::options_description options("Configuration");
        std::map<std::string, const Setting*> settings;
        for (const auto& setting : catalogue_.settings()) {
            if (const auto& name = setting.names().env_cfg) {
                options.add_options()(name->c_str(), po::value<std::string>());
                settings.emplace(*name, &setting);
            }
        }
        po::variables_map vm;
        po::store(po::parse_config_file(in, options), vm);
        if (in.bad()) {
            throw std::runtime_error("Error reading configuration file");
        }

        Layer layer(layers::env_cfg);
        for (const auto& [name, setting] : settings) {
            if (vm.count(name) != 0) {
                const auto& text = vm[name].as<std::string>();
                try {
                    layer.entries.emplace(setting->key(), setting->parse(text));
                }
                catch (const InvalidValue& e) {
                    throw ConfigurationError(prefix + name + ": invalid value " + e.value() + ": " + e.reason());
                }
            }
        }
        return layer;
    }
    catch (const ConfigurationError&) {
        throw;
    }
    catch (const std::exception& e) {
        throw ConfigurationError(prefix + e.what());
    }
}

} // namespace ecf::cfg
