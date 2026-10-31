// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace ecf::cfg {

///
/// @brief Holds the typed value of a configuration entry.
///
using Value = std::variant<bool, std::int64_t, std::string>;

///
/// @brief Implementation details of the configuration, used to decide how a value is stored.
///
/// These traits implement the rule applied by Configuration::set: every argument type is mapped to exactly one
/// alternative of Value, decided at compile time, so that neither the implicit conversions of `std::variant` nor
/// those of the language (e.g. a string literal converting to `bool`) can select the stored alternative.
///
namespace detail {

///
/// @brief Removes the reference and the `const`/`volatile` qualifiers of a type.
///
/// Gives the type a value of @p T is stored as, independently of how it is passed: `T`, `const T&` and `T&&` all
/// yield `T`. Equivalent to C++20 `std::remove_cvref_t`.
///
/// @tparam T The type, possibly a reference and possibly qualified
///
template <typename T>
using bare_t = std::remove_cv_t<std::remove_reference_t<T>>;

///
/// @brief Indicates whether a type is a character type.
///
/// The character types are `char`, `signed char`, `unsigned char`, `wchar_t`, `char16_t` and `char32_t`. They are
/// integral types, but a value of one of them is meant as a character, not as a number: storing `'x'` as the
/// integer 120 would silently change its meaning. They are therefore excluded from is_storable_integer_v.
///
/// @tparam T The type, without reference or qualifiers (see bare_t)
///
template <typename T>
inline constexpr bool is_character_v =
    std::is_same_v<T, char> || std::is_same_v<T, signed char> || std::is_same_v<T, unsigned char> ||
    std::is_same_v<T, wchar_t> || std::is_same_v<T, char16_t> || std::is_same_v<T, char32_t>;

///
/// @brief Indicates whether a type is an integer type whose every value fits in `std::int64_t`.
///
/// The type must be integral, and neither `bool` (stored as `bool`) nor a character type (see is_character_v).
/// Every signed integer type qualifies; an unsigned integer type qualifies only when narrower than 64 bits, since
/// the values of `std::uint64_t` (and of `std::size_t` on 64-bit platforms) above the maximum of `std::int64_t`
/// cannot be represented.
///
/// Examples:
///
///   - accepted: `int`, `long`, `std::int64_t`, `unsigned`, `std::uint32_t`;
///   - refused:  `bool`, `char`, `unsigned char`, `std::uint64_t`, `std::size_t`, `double`.
///
/// @tparam T The type, without reference or qualifiers (see bare_t)
///
template <typename T>
inline constexpr bool is_storable_integer_v = std::is_integral_v<T> && !std::is_same_v<T, bool> && !is_character_v<T> &&
                                              (std::is_signed_v<T> || sizeof(T) < sizeof(std::int64_t));

///
/// @brief Indicates whether a value of a type can be passed to Configuration::set.
///
/// The accepted types, and the alternative of Value each one is stored as, are:
///
///   - Value:                                                 stored as is;
///   - `bool`:                                                `bool`;
///   - an integer type accepted by is_storable_integer_v:     `std::int64_t`;
///   - a type convertible to `std::string_view`, such as a
///     string literal, `const char*`, `std::string` or
///     `std::string_view`:                                     `std::string`.
///
/// Configuration::set refuses, at compile time, any type for which this trait is false.
///
/// @tparam T The type of the argument, possibly a reference and possibly qualified
///
template <typename T>
inline constexpr bool is_settable_v = std::is_same_v<bare_t<T>, Value> || std::is_same_v<bare_t<T>, bool> ||
                                      is_storable_integer_v<bare_t<T>> || std::is_convertible_v<T, std::string_view>;

} // namespace detail

///
/// @brief Names of the layers of configuration, by ecFlow convention.
///
namespace layers {

inline constexpr std::string_view defaults = "defaults"; ///< The built-in defaults
inline constexpr std::string_view env_cfg  = "env.cfg";  ///< The file `server_environment.cfg`
inline constexpr std::string_view ecf_cfg  = "ecf.cfg";  ///< The file `<host>.<port>.ecf.cfg`
inline constexpr std::string_view env      = "env";      ///< The environment variables
inline constexpr std::string_view cli      = "cli";      ///< The command-line options

} // namespace layers

///
/// @brief Reports an invalid configuration key or layer, or an invalid access to an entry.
///
struct ConfigurationError : public std::runtime_error
{
    using std::runtime_error::runtime_error;
};

///
/// @brief Identifies a configuration entry: an option, or a variable.
///
/// A key is a string, of one of the following forms:
///
///   - `<path>`:         an option, e.g. `log.extras`;
///   - `<path>@<NAME>`:  a variable, e.g. `server.variables@ECF_PORT`.
///
/// Notes:
///
///   - A path consists of dot-separated segments, each made of lower-case letters, digits and underscores.
///   - A variable name consists of letters, digits and underscores.
///   - Every key is valid: creating a key from malformed text throws.
///
class Key {
public:
    ///
    /// @brief Creates a key from its text.
    ///
    /// @param[in] key The text of the key, e.g. `log.extras` or `server.variables@ECF_PORT`
    /// @throws ConfigurationError if the text is not a well-formed key.
    ///
    explicit Key(std::string key);

    ///
    /// @brief Creates the key of an option.
    ///
    /// @param[in] path The path of the option, e.g. `log.extras`
    /// @return The key `<path>`.
    /// @throws ConfigurationError if the path is malformed.
    ///
    static Key for_option(std::string_view path);

    ///
    /// @brief Creates the key of a variable.
    ///
    /// @param[in] path The path of the variable, e.g. `server.variables`
    /// @param[in] name The name of the variable, e.g. `ECF_PORT`
    /// @return The key `<path>@<name>`.
    /// @throws ConfigurationError if the path or the name is malformed.
    ///
    static Key for_variable(std::string_view path, std::string_view name);

    ///
    /// @brief Returns the text of the key.
    ///
    /// @return The text, e.g. `server.variables@ECF_PORT`.
    ///
    [[nodiscard]] const std::string& key() const { return key_; }

    ///
    /// @brief Indicates whether the key identifies a variable.
    ///
    /// @return True when the key holds `@`; false for an option.
    ///
    [[nodiscard]] bool is_variable() const;

    ///
    /// @brief Splits the key into its path and its variable name.
    ///
    /// @return The path and the name, as views into this key:
    ///         - for a variable, the text before and after `@`, e.g. `server.variables` and `ECF_PORT`;
    ///         - for an option, the whole text and an empty name.
    ///
    [[nodiscard]] std::pair<std::string_view, std::string_view> components() const;

    ///
    /// @brief Compares the texts of two keys.
    ///
    /// @param[in] lhs The first key
    /// @param[in] rhs The second key
    /// @return True when the texts are equal.
    ///
    friend bool operator==(const Key& lhs, const Key& rhs) { return lhs.key_ == rhs.key_; }

    ///
    /// @brief Compares the texts of two keys.
    ///
    /// @param[in] lhs The first key
    /// @param[in] rhs The second key
    /// @return True when the texts differ.
    ///
    friend bool operator!=(const Key& lhs, const Key& rhs) { return lhs.key_ != rhs.key_; }

    ///
    /// @brief Orders two keys by their texts.
    ///
    /// @param[in] lhs The first key
    /// @param[in] rhs The second key
    /// @return True when the text of @p lhs comes first.
    ///
    friend bool operator<(const Key& lhs, const Key& rhs) { return lhs.key_ < rhs.key_; }

private:
    std::string key_;
};

///
/// @brief Holds the entries provided by one source of configuration.
///
struct Layer
{
    ///
    /// @brief Creates a layer.
    ///
    /// @param[in] name The name of the layer, e.g. ecf::cfg::layers::env
    /// @param[in] entries The entries of the layer; empty by default
    ///
    explicit Layer(std::string_view name, std::map<Key, Value> entries = {})
        : name{name},
          entries{std::move(entries)} {}

    std::string name;             ///< The name of the layer
    std::map<Key, Value> entries; ///< The values held by the layer, by key
};

///
/// @brief Stores pre-calculated configuration values as layers of entries, one layer per source.
///
/// The layers are declared at construction, from lowest to highest precedence. The effective value of a key is
/// taken from the highest layer holding it, whatever the order in which the layers are filled; the value of a
/// given layer can also be selected explicitly.
///
/// @invariant The layer names are unique, and their order never changes after construction.
///
class Configuration {
public:
    ///
    /// @brief Creates an empty configuration.
    ///
    /// @param[in] layers The layers, from lowest to highest precedence, normally empty; no convention is imposed on
    ///                   their names, which only need to be non-empty and unique
    /// @throws ConfigurationError if no layer is given, or if a layer name is empty or repeated.
    ///
    explicit Configuration(std::vector<Layer> layers);

    ///
    /// @brief Sets the value of a key in a layer.
    ///
    /// The value is stored as exactly one alternative of Value, according to its type:
    ///
    ///   - Value:                                            stored as is;
    ///   - `bool`:                                           `bool`;
    ///   - an integer type, except the character types and
    ///     the unsigned types of 64 bits or more:            `std::int64_t`;
    ///   - a type convertible to `std::string_view`:        `std::string`; a string literal is never a `bool`.
    ///
    /// Any other type does not compile.
    ///
    /// @tparam T The type of the value
    /// @param[in] layer The name of the layer, e.g. ecf::cfg::layers::env
    /// @param[in] key The key
    /// @param[in] value The value
    /// @throws ConfigurationError if the layer is unknown, or already holds the key.
    ///
    template <typename T>
    void set(std::string_view layer, const Key& key, T&& value) {
        static_assert(detail::is_settable_v<T>,
                      "A configuration value is a Value, a bool, an integer fitting in std::int64_t (not a "
                      "character), or text convertible to std::string_view");
        using B = detail::bare_t<T>;
        if constexpr (std::is_same_v<B, Value>) {
            store(layer, key, std::forward<T>(value));
        }
        else if constexpr (std::is_same_v<B, bool>) {
            store(layer, key, Value(std::in_place_type<bool>, value));
        }
        else if constexpr (detail::is_storable_integer_v<B>) {
            store(layer, key, Value(std::in_place_type<std::int64_t>, static_cast<std::int64_t>(value)));
        }
        else {
            store(layer, key, Value(std::in_place_type<std::string>, std::string_view(value)));
        }
    }

    ///
    /// @brief Returns the effective value of a key.
    ///
    /// @param[in] key The key
    /// @return The value of the highest layer holding the key; empty when no layer holds it.
    ///
    [[nodiscard]] std::optional<Value> get(const Key& key) const;

    ///
    /// @brief Returns the value of a key in the given layer, regardless of precedence.
    ///
    /// @param[in] key The key
    /// @param[in] layer The name of the layer
    /// @return The value held by the layer; empty when the layer does not hold the key.
    /// @throws ConfigurationError if the layer is unknown.
    ///
    [[nodiscard]] std::optional<Value> get(const Key& key, std::string_view layer) const;

    ///
    /// @brief Returns the name of the layer providing the effective value of a key.
    ///
    /// @param[in] key The key
    /// @return The name of the highest layer holding the key; empty when no layer holds it.
    ///
    [[nodiscard]] std::optional<std::string> source(const Key& key) const;

    ///
    /// @brief Returns the keys held by any layer.
    ///
    /// @return Each key once, ordered by text.
    ///
    [[nodiscard]] std::vector<Key> keys() const;

private:
    void store(std::string_view layer, const Key& key, Value value);
    [[nodiscard]] Layer& layer(std::string_view name);
    [[nodiscard]] const Layer& layer(std::string_view name) const;

    // The highest layer holding the key, and its value; both null when no layer holds the key
    [[nodiscard]] std::pair<const Layer*, const Value*> effective(const Key& key) const;

    std::vector<Layer> layers_; ///< From lowest to highest precedence
};

} // namespace ecf::cfg
