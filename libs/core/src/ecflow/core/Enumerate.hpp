/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_core_Enumerate_HPP
#define ecflow_core_Enumerate_HPP

#include <algorithm>
#include <array>
#include <cassert>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace ecf {

namespace detail {

///
/// @brief EnumTraits defines the mapping between a set of enum values and their designation.
///
/// EnumTraits must define:
///  - the mapping `map`, provided as a std::array composed of a std::pair<E, const char *> for each enum value
///  - the `size`, holding the number of entries in `map`
///
/// @tparam E the enum type for which the traits are defined
///
template <typename E>
struct EnumTraits
{
};

} // namespace detail

///
/// @brief Enumerate provides bidirectional mapping between enum values and their string designations.
///
/// @tparam E      the enum type to map
/// @tparam TRAITS the traits type providing the mapping; defaults to detail::EnumTraits<E>
///
template <typename E, typename TRAITS = detail::EnumTraits<E>>
struct Enumerate
{
public:
    using enum_t   = E;
    using string_t = std::string_view;

    ///
    /// @brief Convert the given enum value to its designation.
    ///
    /// This is an "unsafe" operation, as it assumes that the given enum value exists in the mapping.
    /// If the enum value does not exist, an assertion failure is raised.
    ///
    /// @param e the enum value
    /// @return the associated designation
    ///
    static constexpr string_t as_string(enum_t e) noexcept {
        auto found = std::find_if(
            std::begin(TRAITS::map), std::end(TRAITS::map), [&](const auto& item) { return item.first == e; });

        assert(found != std::end(TRAITS::map));

        return found->second;
    }

    ///
    /// @brief Convert the given enum value to its designation.
    ///
    /// @param e the enum value
    /// @return the associated designation, in case it exists; an empty optional, otherwise
    ///
    static constexpr std::optional<string_t> to_string(enum_t e) noexcept {
        if (auto found = std::find_if(
                std::begin(TRAITS::map), std::end(TRAITS::map), [&](const auto& item) { return item.first == e; });
            found != std::end(TRAITS::map)) {

            return std::make_optional(found->second);
        }
        return std::nullopt;
    }

    ///
    /// @brief Convert the given designation to the related enum value.
    ///
    /// @param s the designation
    /// @return the enum value, in case it exists; an empty optional, otherwise
    ///
    static constexpr std::optional<E> to_enum(string_t s) noexcept {
        if (auto found = std::find_if(
                std::begin(TRAITS::map), std::end(TRAITS::map), [&](const auto& item) { return item.second == s; });
            found != std::end(TRAITS::map)) {

            return std::make_optional(found->first);
        }
        return std::nullopt;
    }

    ///
    /// @brief Check if the given designation is valid (i.e. has a related enum value).
    ///
    /// @param s the designation
    /// @return true, if valid; false, otherwise
    ///
    static constexpr bool is_valid(string_t s) {
        auto found = std::find_if(
            std::begin(TRAITS::map), std::end(TRAITS::map), [&](const auto& item) { return item.second == s; });
        return found != std::end(TRAITS::map);
    }

    ///
    /// @brief The number of mapped enum values.
    ///
    static const size_t size = TRAITS::size;

    ///
    /// @brief Collect all mapped enum values.
    ///
    /// @return a vector containing all mapped enum values
    ///
    static auto enums() {
        std::vector<E> result;
        result.reserve(TRAITS::size);
        std::transform(std::begin(TRAITS::map),
                       std::end(TRAITS::map),
                       std::back_inserter(result),
                       [](const auto& item) { return item.first; });
        return result;
    }

    ///
    /// @brief Collect all mapped designations.
    ///
    /// @return a vector containing all mapped designations
    ///
    static auto designations() {
        std::vector<std::string> result;
        result.reserve(TRAITS::size);
        std::transform(std::begin(TRAITS::map),
                       std::end(TRAITS::map),
                       std::back_inserter(result),
                       [](const auto& item) { return std::string{item.second}; });

        return result;
    }
};

} // namespace ecf

#endif /* ecflow_core_Enumerate_HPP */
