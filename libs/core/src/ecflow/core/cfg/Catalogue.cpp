// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include "ecflow/core/cfg/Catalogue.hpp"

#include <algorithm>
#include <charconv>
#include <utility>

namespace ecf::cfg {

namespace {

std::string quoted(std::string_view text) {
    return "'" + std::string(text) + "'";
}

const char* type_name(Type type) {
    switch (type) {
        case Type::Boolean:
            return "a boolean";
        case Type::Integer:
            return "an integer";
        case Type::Text:
            return "a text";
    }
    return "a value";
}

Type type_of(const Value& value) {
    static constexpr Type types[] = {Type::Boolean, Type::Integer, Type::Text};
    return types[value.index()];
}

std::string describe(const Value& value) {
    if (const auto* b = std::get_if<bool>(&value)) {
        return *b ? "true" : "false";
    }
    if (const auto* i = std::get_if<std::int64_t>(&value)) {
        return std::to_string(*i);
    }
    return quoted(std::get<std::string>(value));
}

std::string join(const std::vector<std::string>& texts) {
    std::string joined;
    for (const auto& text : texts) {
        joined += (joined.empty() ? "" : ", ") + text;
    }
    return joined;
}

} // namespace

std::string path_of(const Key& key) {
    const auto [path, name] = key.components();
    std::string pointer     = "/" + std::string(path);
    std::replace(pointer.begin(), pointer.end(), '.', '/');
    return name.empty() ? pointer : pointer + "/" + std::string(name);
}

InvalidValue::InvalidValue(Key key, std::string value, std::string reason)
    : ConfigurationError("Invalid value " + value + " for " + quoted(key.key()) + ": " + reason),
      key_{std::move(key)},
      value_{std::move(value)},
      reason_{std::move(reason)} {
}

//
// Checks
//

namespace checks {

Check range(std::int64_t min, std::int64_t max) {
    return [min, max](const Value& value) -> std::optional<std::string> {
        const auto v = std::get<std::int64_t>(value);
        if (v < min || v > max) {
            return "expected a value in [" + std::to_string(min) + ", " + std::to_string(max) + "]";
        }
        return std::nullopt;
    };
}

Check at_least(std::int64_t min) {
    return [min](const Value& value) -> std::optional<std::string> {
        if (std::get<std::int64_t>(value) < min) {
            return "expected a value at least " + std::to_string(min);
        }
        return std::nullopt;
    };
}

Check non_empty() {
    return [](const Value& value) -> std::optional<std::string> {
        if (std::get<std::string>(value).empty()) {
            return std::string("expected a non-empty value");
        }
        return std::nullopt;
    };
}

Check one_of(std::vector<std::string> accepted) {
    return [accepted = std::move(accepted)](const Value& value) -> std::optional<std::string> {
        const auto& v = std::get<std::string>(value);
        if (std::find(accepted.begin(), accepted.end(), v) == accepted.end()) {
            return "expected one of " + join(accepted);
        }
        return std::nullopt;
    };
}

} // namespace checks

//
// Setting
//

Setting::Setting(Key key, Type type, std::optional<Value> default_value, Names names, Check check, Parse parse)
    : key_{std::move(key)},
      type_{type},
      default_value_{std::move(default_value)},
      names_{std::move(names)},
      check_{std::move(check)},
      parse_{std::move(parse)} {
    if (names_.ecf_cfg && *names_.ecf_cfg != path_of(key_)) {
        throw ConfigurationError("Configuration setting " + quoted(key_.key()) + " is named " +
                                 quoted(*names_.ecf_cfg) + " in the configuration file, instead of " +
                                 quoted(path_of(key_)));
    }
    if (default_value_) {
        this->check(*default_value_); // the parameter named check is moved into check_
    }
}

Value Setting::parse(std::string_view text) const {
    auto refuse = [this, text](const std::string& reason) { return InvalidValue(key_, quoted(text), reason); };

    Value value;
    if (parse_) {
        value = parse_(text);
    }
    else {
        switch (type_) {
            case Type::Integer: {
                std::int64_t number  = 0;
                const auto* first    = text.data();
                const auto* last     = text.data() + text.size();
                const auto [end, ec] = std::from_chars(first, last, number);
                if (text.empty() || ec != std::errc{} || end != last) {
                    throw refuse("expected an integer");
                }
                value = Value(std::in_place_type<std::int64_t>, number);
                break;
            }
            case Type::Boolean:
                if (text == "true") {
                    value = Value(std::in_place_type<bool>, true);
                }
                else if (text == "false") {
                    value = Value(std::in_place_type<bool>, false);
                }
                else {
                    throw refuse("expected 'true' or 'false'");
                }
                break;
            case Type::Text:
                value = Value(std::in_place_type<std::string>, text);
                break;
        }
    }
    try {
        check(value);
    }
    catch (const InvalidValue& e) {
        throw refuse(e.reason()); // shows the text as given
    }
    return value;
}

void Setting::check(const Value& value) const {
    if (type_of(value) != type_) {
        throw InvalidValue(
            key_, describe(value), std::string("expected ") + type_name(type_) + ", not " + type_name(type_of(value)));
    }
    if (check_) {
        if (const auto reason = check_(value)) {
            throw InvalidValue(key_, describe(value), *reason);
        }
    }
}

//
// Catalogue
//

Catalogue::Catalogue(std::vector<Setting> settings)
    : settings_{std::move(settings)} {
    std::sort(settings_.begin(), settings_.end(), [](const Setting& lhs, const Setting& rhs) {
        return lhs.key() < rhs.key();
    });
    const auto same = [](const Setting& lhs, const Setting& rhs) { return lhs.key() == rhs.key(); };
    if (const auto duplicate = std::adjacent_find(settings_.begin(), settings_.end(), same);
        duplicate != settings_.end()) {
        throw ConfigurationError("Configuration setting " + quoted(duplicate->key().key()) +
                                 " is declared more than once");
    }

    std::vector<std::pair<std::string, const Key*>> paths;
    for (const auto& setting : settings_) {
        paths.emplace_back(path_of(setting.key()), &setting.key());
    }
    // As '/' sorts before any character of a key, a path sorts right before the paths inside it
    std::sort(paths.begin(), paths.end());
    for (std::size_t i = 1; i < paths.size(); ++i) {
        const auto& [outer, outer_key] = paths[i - 1];
        const auto& [inner, inner_key] = paths[i];
        if (inner == outer || inner.rfind(outer + "/", 0) == 0) {
            throw ConfigurationError("Configuration settings " + quoted(outer_key->key()) + " and " +
                                     quoted(inner_key->key()) + " are ambiguous: they designate overlapping paths, " +
                                     quoted(outer) + " and " + quoted(inner));
        }
    }
}

const Setting* Catalogue::find(const Key& key) const {
    const auto found =
        std::lower_bound(settings_.begin(), settings_.end(), key, [](const Setting& setting, const Key& k) {
            return setting.key() < k;
        });
    return found != settings_.end() && found->key() == key ? &*found : nullptr;
}

Value Catalogue::parse(const Key& key, std::string_view text) const {
    return get(key).parse(text);
}

void Catalogue::check(const Key& key, const Value& value) const {
    get(key).check(value);
}

Layer defaults_layer(const Catalogue& catalogue) {
    Layer layer(layers::defaults);
    for (const auto& setting : catalogue.settings()) {
        if (setting.default_value()) {
            layer.entries.emplace(setting.key(), *setting.default_value());
        }
    }
    return layer;
}

const Setting& Catalogue::get(const Key& key) const {
    const auto* setting = find(key);
    if (!setting) {
        throw ConfigurationError("Unknown configuration key " + quoted(key.key()));
    }
    return *setting;
}

} // namespace ecf::cfg
