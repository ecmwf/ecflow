// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include "ecflow/core/cfg/Configuration.hpp"

#include <algorithm>
#include <set>
#include <utility>

namespace ecf::cfg {

namespace {

bool is_lower_word(std::string_view text) {
    return !text.empty() && std::all_of(text.begin(), text.end(), [](char c) {
        return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_';
    });
}

bool is_variable_name(std::string_view text) {
    return !text.empty() && std::all_of(text.begin(), text.end(), [](char c) {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_';
    });
}

// A dot-separated sequence of lower-case words, e.g. `env.cfg` or `log.extras`
bool is_dotted_path(std::string_view text) {
    if (text.empty()) {
        return false;
    }
    std::size_t start = 0;
    while (true) {
        const auto dot = text.find('.', start);
        if (!is_lower_word(text.substr(start, dot == std::string_view::npos ? std::string_view::npos : dot - start))) {
            return false;
        }
        if (dot == std::string_view::npos) {
            return true;
        }
        start = dot + 1;
    }
}

bool is_key(std::string_view text) {
    const auto at = text.find('@');
    if (at == std::string_view::npos) {
        return is_dotted_path(text);
    }
    return is_dotted_path(text.substr(0, at)) && is_variable_name(text.substr(at + 1));
}

std::string quoted(std::string_view text) {
    return "'" + std::string(text) + "'";
}

} // namespace

//
// Key
//

Key::Key(std::string key)
    : key_{std::move(key)} {
    if (!is_key(key_)) {
        throw ConfigurationError("Invalid configuration key " + quoted(key_) +
                                 ": expected '<path>' or '<path>@<NAME>', where <path> consists of dot-separated "
                                 "words of lower-case letters, digits and underscores, and <NAME> of letters, "
                                 "digits and underscores");
    }
}

Key Key::for_option(std::string_view path) {
    if (path.find('@') != std::string_view::npos) {
        throw ConfigurationError("Invalid configuration key " + quoted(path) + " of an option: unexpected '@'");
    }
    return Key(std::string(path));
}

Key Key::for_variable(std::string_view path, std::string_view name) {
    return Key(std::string(path) + "@" + std::string(name));
}

bool Key::is_variable() const {
    return key_.find('@') != std::string::npos;
}

std::pair<std::string_view, std::string_view> Key::components() const {
    const std::string_view key = key_;
    const auto at              = key.find('@');
    if (at == std::string_view::npos) {
        return {key, {}};
    }
    return {key.substr(0, at), key.substr(at + 1)};
}

//
// Configuration
//

Configuration::Configuration(std::vector<Layer> layers) {
    if (layers.empty()) {
        throw ConfigurationError("A configuration needs at least one layer");
    }
    for (auto& layer : layers) {
        const auto& name = layer.name;
        if (name.empty()) {
            throw ConfigurationError("Invalid configuration layer: the name of a layer cannot be empty");
        }
        const auto same = [&name](const Layer& other) { return other.name == name; };
        if (std::any_of(layers_.begin(), layers_.end(), same)) {
            throw ConfigurationError("Configuration layer " + quoted(name) + " is declared more than once");
        }
        layers_.push_back(std::move(layer));
    }
}

void Configuration::store(std::string_view layer, const Key& key, Value value) {
    auto& entries = this->layer(layer).entries;
    if (entries.find(key) != entries.end()) {
        throw ConfigurationError("Configuration key " + quoted(key.key()) + " is set more than once in layer " +
                                 quoted(layer));
    }
    entries.emplace(key, std::move(value));
}

std::optional<Value> Configuration::get(const Key& key) const {
    const auto [layer, value] = effective(key);
    if (!value) {
        return std::nullopt;
    }
    return *value;
}

std::optional<Value> Configuration::get(const Key& key, std::string_view layer) const {
    const auto& entries = this->layer(layer).entries;
    const auto found    = entries.find(key);
    if (found == entries.end()) {
        return std::nullopt;
    }
    return found->second;
}

std::optional<std::string> Configuration::source(const Key& key) const {
    const auto [layer, value] = effective(key);
    if (!layer) {
        return std::nullopt;
    }
    return layer->name;
}

std::vector<Key> Configuration::keys() const {
    std::set<Key> keys;
    for (const auto& layer : layers_) {
        for (const auto& [key, value] : layer.entries) {
            keys.insert(key);
        }
    }
    return {keys.begin(), keys.end()};
}

Layer& Configuration::layer(std::string_view name) {
    const auto found =
        std::find_if(layers_.begin(), layers_.end(), [name](const Layer& layer) { return layer.name == name; });
    if (found == layers_.end()) {
        throw ConfigurationError("Unknown configuration layer " + quoted(name));
    }
    return *found;
}

const Layer& Configuration::layer(std::string_view name) const {
    const auto found =
        std::find_if(layers_.begin(), layers_.end(), [name](const Layer& layer) { return layer.name == name; });
    if (found == layers_.end()) {
        throw ConfigurationError("Unknown configuration layer " + quoted(name));
    }
    return *found;
}

std::pair<const Layer*, const Value*> Configuration::effective(const Key& key) const {
    for (auto layer = layers_.rbegin(); layer != layers_.rend(); ++layer) {
        if (const auto found = layer->entries.find(key); found != layer->entries.end()) {
            return {&*layer, &found->second};
        }
    }
    return {nullptr, nullptr};
}

} // namespace ecf::cfg
