/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/client/HelpCatalog.hpp"

#include "ecflow/client/generated_client_help.hpp"

namespace ecf {

const nlohmann::json& HelpCatalog::manifest() {
    static const nlohmann::json instance = nlohmann::json::parse(client_help_json);
    return instance;
}

const nlohmann::json* HelpCatalog::find_command(const std::string& name) {
    return find_by_name(manifest().at("commands"), name);
}

const nlohmann::json* HelpCatalog::find_option(const std::string& name) {
    return find_by_name(manifest().at("options"), name);
}

const nlohmann::json* HelpCatalog::find_topic(const std::string& name) {
    return find_by_name(manifest().at("topics"), name);
}

const nlohmann::json* HelpCatalog::find_by_name(const nlohmann::json& array, const std::string& name) {
    for (const auto& entry : array) {
        if (entry.at("name") == name) {
            return &entry;
        }
    }
    return nullptr;
}

} // namespace ecf
