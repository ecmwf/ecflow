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
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace ecf {

namespace detail {

/**
 * EnumTraits defines the mapping between a set of enum values and their designation.
 *
 * EnumTraits must define:
 *  - the mapping `map`, provided as an std::array composes of an std::pair<E, const char *> for each enum value
 *  - the `size`, holding the number os entries in `map`
 *
 * @tparam E
 */

template <typename E>
struct EnumTraits
{
};

} // namespace detail

template <typename E, typename TRAITS = detail::EnumTraits<E>>
struct Enumerate
{
public:
    using enum_t   = E;
    using string_t = std::string_view;

    /**
     * Convert the given enum value to its designation
     *
     * @param e the enum value
     * @return the designation, in case it exists; an empty optional, otherwise
     */
    static constexpr inline std::optional<string_t> to_string(enum_t e) noexcept {
        auto found = std::find_if(
            std::begin(TRAITS::map), std::end(TRAITS::map), [&](const auto& item) { return item.first == e; });
        if (found == std::end(TRAITS::map)) {
            return std::nullopt;
        }
        return std::make_optional(found->second);
    }

    /**
     * Convert the given designation to the related enum value
     *
     * @param s the designation
     * @return the enum value, in case it exists; an empty optional, otherwise
     */
    static constexpr inline std::optional<E> to_enum(string_t s) noexcept {
        auto found = std::find_if(
            std::begin(TRAITS::map), std::end(TRAITS::map), [&](const auto& item) { return item.second == s; });
        if (found == std::end(TRAITS::map)) {
            return std::nullopt;
        }
        return std::make_optional(found->first);
    }

    /**
     * Checks if the given designation is valid (i.e. has a related enum value)
     *
     * @param s the designation
     * @return true, if valid; false, otherwise
     */
    static bool inline is_valid(string_t s) {
        auto found = std::find_if(
            std::begin(TRAITS::map), std::end(TRAITS::map), [&](const auto& item) { return item.second == s; });
        return found != std::end(TRAITS::map);
    }

    /**
     * The number of mapped enum values
     */
    static const size_t size = TRAITS::size;

    /**
     * The vector of mapped enum values
     */
    static auto enums() {
        std::vector<E> result;
        result.reserve(TRAITS::size);
        std::transform(std::begin(TRAITS::map),
                       std::end(TRAITS::map),
                       std::back_inserter(result),
                       [](const auto& item) { return item.first; });
        return result;
    }

    /**
     * The vector of mapped designations
     */
    static auto designations() {
        std::vector<std::string> result;
        result.reserve(TRAITS::size);
        std::transform(std::begin(TRAITS::map),
                       std::end(TRAITS::map),
                       std::back_inserter(result),
                       [](const auto& item) { return item.second; });

        return result;
    }
};

} // namespace ecf

#endif /* ecflow_core_Enumerate_HPP */
