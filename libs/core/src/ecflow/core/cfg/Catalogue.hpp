// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "ecflow/core/cfg/Configuration.hpp"

namespace ecf::cfg {

///
/// @brief Identifies the alternative of Value held by the values of a setting.
///
enum class Type {
    Boolean, ///< `bool`
    Integer, ///< `std::int64_t`
    Text     ///< `std::string`
};

///
/// @brief Names an input in each of the origins that can provide it, as named by layers.
///
/// An origin that cannot provide the input leaves its name empty.
///
struct Names
{
    std::optional<std::string> env_cfg; ///< The setting in `server_environment.cfg`, e.g. `ECF_PORT`
    std::optional<std::string> ecf_cfg; ///< The path in `<host>.<port>.ecf.cfg`, e.g. `/.../variables/ECF_SSL`
    std::optional<std::string> env;     ///< The environment variable, e.g. `ECF_PORT`
    std::optional<std::string> cli;     ///< The command-line option, without dashes, e.g. `port`
};

///
/// @brief Returns the path designated by a key in a configuration file.
///
/// The path is a JSON Pointer (RFC 6901) made of the segments of the path of the key, followed by its variable name
/// when it has one.
///
/// @param[in] key The key
/// @return The path, e.g. `/a/b/c` for both the option `a.b.c` and the variable `a.b@c`.
///
[[nodiscard]] std::string path_of(const Key& key);

///
/// @brief Checks a value of the expected type.
///
/// The callable returns the reason a value is refused, e.g. `expected a value in [1024, 49150]`, or nothing when
/// the value is accepted.
///
using Check = std::function<std::optional<std::string>(const Value&)>;

///
/// @brief Turns the text provided by a source into a value.
///
/// The callable throws ConfigurationError when the text cannot be turned into a value.
///
using Parse = std::function<Value(std::string_view)>;

///
/// @brief Common checks of setting values.
///
namespace checks {

///
/// @brief Creates a check accepting an integer in a closed interval.
///
/// @param[in] min The smallest accepted value
/// @param[in] max The largest accepted value
/// @return The check.
///
Check range(std::int64_t min, std::int64_t max);

///
/// @brief Creates a check accepting an integer not below a minimum.
///
/// @param[in] min The smallest accepted value
/// @return The check.
///
Check at_least(std::int64_t min);

///
/// @brief Creates a check accepting non-empty text.
///
/// @return The check.
///
Check non_empty();

///
/// @brief Creates a check accepting one of the given texts, compared exactly.
///
/// @param[in] accepted The accepted texts
/// @return The check.
///
Check one_of(std::vector<std::string> accepted);

} // namespace checks

///
/// @brief Describes one configuration input: its key, type, default value, check and names in the sources.
///
/// @invariant The default value, when present, holds the type of the setting and passes its check.
///
class Setting {
public:
    ///
    /// @brief Creates a setting.
    ///
    /// @param[in] key The key of the setting, e.g. `ecflow.server.variables@ECF_PORT`
    /// @param[in] type The type of the values of the setting
    /// @param[in] default_value The built-in default; empty when the setting has none
    /// @param[in] names The names of the input in the sources that can provide it
    /// @param[in] check The check of the values; none when every value of the type is accepted
    /// @param[in] parse The rule turning text into a value; none to apply the rule of the type (see parse())
    /// @throws ConfigurationError if the default value does not hold the type, or does not pass the check, or if the
    ///   name in `<host>.<port>.ecf.cfg` is not the path designated by the key (see path_of()).
    ///
    Setting(Key key, Type type, std::optional<Value> default_value, Names names, Check check = {}, Parse parse = {});

    ///
    /// @brief Returns the key of the setting.
    ///
    /// @return The key.
    ///
    [[nodiscard]] const Key& key() const { return key_; }

    ///
    /// @brief Returns the type of the values of the setting.
    ///
    /// @return The type.
    ///
    [[nodiscard]] Type type() const { return type_; }

    ///
    /// @brief Returns the built-in default of the setting.
    ///
    /// @return The default value; empty when the setting has none.
    ///
    [[nodiscard]] const std::optional<Value>& default_value() const { return default_value_; }

    ///
    /// @brief Returns the names of the input in the sources.
    ///
    /// @return The names.
    ///
    [[nodiscard]] const Names& names() const { return names_; }

    ///
    /// @brief Turns the text provided by a source into a checked value.
    ///
    /// Without a custom rule, the text is turned into a value according to the type:
    ///
    ///   - Integer: a decimal integer, optionally preceded by `-`, without spaces or other characters;
    ///   - Boolean: exactly `true` or `false`;
    ///   - Text:    the text, unchanged.
    ///
    /// The resulting value then passes the check of the setting.
    ///
    /// @param[in] text The text provided by the source
    /// @return The value.
    /// @throws ConfigurationError if the text cannot be turned into a value, or the value does not pass the check.
    ///
    [[nodiscard]] Value parse(std::string_view text) const;

    ///
    /// @brief Checks a value of the setting.
    ///
    /// @param[in] value The value
    /// @throws ConfigurationError if the value does not hold the type of the setting, or does not pass its check.
    ///
    void check(const Value& value) const;

private:
    Key key_;
    Type type_;
    std::optional<Value> default_value_;
    Names names_;
    Check check_;
    Parse parse_;
};

///
/// @brief Holds the settings known to a component, by key.
///
/// Each key designates a path in a configuration file (see path_of()), e.g. `/a/b/c` for both the option `a.b.c`
/// and the variable `a.b@c`.
///
/// @invariant Each key appears once, and the settings are ordered by key.
/// @invariant No two keys designate the same path, and no key designates a path inside that of another.
///
class Catalogue {
public:
    ///
    /// @brief Creates a catalogue.
    ///
    /// @param[in] settings The settings, in any order
    /// @throws ConfigurationError if a key appears more than once, or if two keys designate the same path, or
    ///   paths one inside the other (e.g. `a.b.c` and `a.b@c`, or `a.b` and `a.b@c`).
    ///
    explicit Catalogue(std::vector<Setting> settings);

    ///
    /// @brief Returns the settings.
    ///
    /// @return The settings, ordered by key.
    ///
    [[nodiscard]] const std::vector<Setting>& settings() const { return settings_; }

    ///
    /// @brief Finds the setting with the given key.
    ///
    /// @param[in] key The key
    /// @return The setting; nullptr when the catalogue has no setting with this key.
    ///
    [[nodiscard]] const Setting* find(const Key& key) const;

    ///
    /// @brief Turns the text provided by a source into a checked value of the setting with the given key.
    ///
    /// @param[in] key The key
    /// @param[in] text The text provided by the source
    /// @return The value.
    /// @throws ConfigurationError if the key is unknown, or if Setting::parse() throws.
    ///
    [[nodiscard]] Value parse(const Key& key, std::string_view text) const;

    ///
    /// @brief Checks a value of the setting with the given key.
    ///
    /// @param[in] key The key
    /// @param[in] value The value
    /// @throws ConfigurationError if the key is unknown, or if Setting::check() throws.
    ///
    void check(const Key& key, const Value& value) const;

private:
    [[nodiscard]] const Setting& get(const Key& key) const;

    std::vector<Setting> settings_; ///< Ordered by key
};

} // namespace ecf::cfg
