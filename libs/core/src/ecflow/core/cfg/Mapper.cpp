// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include "ecflow/core/cfg/Mapper.hpp"

#include <cstdint>
#include <optional>
#include <utility>
#include <variant>

namespace ecf::cfg {

namespace {

ConfigurationError refuse(std::string_view origin, const std::string& path, const std::string& reason) {
    return ConfigurationError(std::string(origin) + ": " + (path.empty() ? "" : path + ": ") + reason);
}

const char* type_name(Type type) {
    switch (type) {
        case Type::Boolean:
            return "a boolean";
        case Type::Integer:
            return "an integer";
        case Type::Text:
            return "a string";
    }
    return "a value";
}

const char* kind_of(const CTree& tree) {
    static constexpr const char* kinds[] = {
        "null", "a boolean", "an integer", "a decimal number", "a string", "an array", "an object"};
    return kinds[tree.node().index()];
}

// The value of the node, when it holds the native type of the setting
std::optional<Value> value_of(const CTree& tree, Type type) {
    const auto& node = tree.node();
    switch (type) {
        case Type::Boolean:
            if (const auto* b = std::get_if<bool>(&node)) {
                return Value(std::in_place_type<bool>, *b);
            }
            break;
        case Type::Integer:
            if (const auto* i = std::get_if<std::int64_t>(&node)) {
                return Value(std::in_place_type<std::int64_t>, *i);
            }
            break;
        case Type::Text:
            if (const auto* s = std::get_if<std::string>(&node)) {
                return Value(std::in_place_type<std::string>, *s);
            }
            break;
    }
    return std::nullopt;
}

} // namespace

Mapper::Mapper(const Catalogue& catalogue) {
    for (const auto& setting : catalogue.settings()) {
        if (const auto& path = setting.names().ecf_cfg) {
            settings_.emplace(*path, &setting);
            for (auto slash = path->find('/', 1); slash != std::string::npos; slash = path->find('/', slash + 1)) {
                objects_.insert(path->substr(0, slash));
            }
        }
    }
}

Layer Mapper::map(const CTree& tree, std::string_view layer, std::string_view origin) const {
    if (!std::holds_alternative<CTree::Mapping>(tree.node())) {
        throw refuse(origin, tree.path(), std::string("expected an object, not ") + kind_of(tree));
    }

    const auto* version = tree.find("version");
    if (!version) {
        throw refuse(origin, tree.path(), "missing the version of the format, expected \"version\": 1");
    }
    const auto* number = std::get_if<std::int64_t>(&version->node());
    if (!number || *number != 1) {
        throw refuse(origin, version->path(), "unsupported version of the format, expected 1");
    }

    std::map<Key, Value> entries;
    map_object(tree, entries, origin);
    return Layer(layer, std::move(entries));
}

void Mapper::map_object(const CTree& tree, std::map<Key, Value>& entries, std::string_view origin) const {
    for (const auto& [name, member] : std::get<CTree::Mapping>(tree.node())) {
        if (tree.path().empty() && name == "version") {
            continue; // Checked by map()
        }

        // The path of the member, as a JSON Pointer, is the name of its setting in the file
        if (const auto found = settings_.find(member.path()); found != settings_.end()) {
            const auto& setting = *found->second;
            auto value          = value_of(member, setting.type());
            if (!value) {
                throw refuse(origin,
                             member.path(),
                             std::string("expected ") + type_name(setting.type()) + ", not " + kind_of(member));
            }
            try {
                setting.check(*value);
            }
            catch (const InvalidValue& e) {
                throw refuse(origin, member.path(), "invalid value " + e.value() + ": " + e.reason());
            }
            entries.emplace(setting.key(), std::move(*value));
        }
        else if (objects_.count(member.path()) != 0) {
            if (!std::holds_alternative<CTree::Mapping>(member.node())) {
                throw refuse(origin, member.path(), std::string("expected an object, not ") + kind_of(member));
            }
            map_object(member, entries, origin);
        }
        else {
            throw refuse(origin, member.path(), "unknown setting");
        }
    }
}

} // namespace ecf::cfg
