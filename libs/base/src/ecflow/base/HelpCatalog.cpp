/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/base/HelpCatalog.hpp"

#include "ecflow/base/generated_client_help.hpp"

namespace ecf {

const std::string HelpCatalog::not_provided = "No description available";

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

const nlohmann::json* HelpCatalog::find_definition_item(const std::string& name) {
    return find_by_name(manifest().at("definitions"), name);
}

std::optional<std::string> HelpCatalog::summary_for(const std::string& name) {
    if (const nlohmann::json* entry = entry_for(name)) {
        return entry->at("summary").get<std::string>();
    }
    return std::nullopt;
}

std::optional<std::string> HelpCatalog::description_for(const std::string& name) {
    const nlohmann::json* entry = entry_for(name);
    if (!entry) {
        return std::nullopt;
    }
    return join_description(entry->at("description"));
}

std::optional<std::string> HelpCatalog::summary_for_definition_item(const std::string& name) {
    if (const nlohmann::json* entry = find_definition_item(name)) {
        return entry->at("summary").get<std::string>();
    }
    return std::nullopt;
}

std::optional<std::string> HelpCatalog::description_for_definition_item(const std::string& name) {
    const nlohmann::json* entry = find_definition_item(name);
    if (!entry) {
        return std::nullopt;
    }
    return join_description(entry->at("description"));
}

std::string HelpCatalog::join_description(const nlohmann::json& description) {
    std::string text;
    bool first = true;
    for (const auto& line : description) {
        if (!first) {
            text += "\n";
        }
        first = false;
        text += line.get<std::string>();
    }
    return text;
}

const nlohmann::json* HelpCatalog::find_by_name(const nlohmann::json& array, const std::string& name) {
    for (const auto& entry : array) {
        if (entry.at("name") == name) {
            return &entry;
        }
    }
    return nullptr;
}

const nlohmann::json* HelpCatalog::entry_for(const std::string& name) {
    if (const nlohmann::json* command = find_command(name)) {
        return command;
    }
    return find_option(name);
}

} // namespace ecf
