/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_core_Converter_HPP
#define ecflow_core_Converter_HPP

#include <string>
#include <utility>

#include <boost/lexical_cast.hpp>

namespace ecf {

///
/// @brief Exception thrown when a value fails to convert between types via convert_to.
///
/// @see convert_to
///
struct bad_conversion : public std::runtime_error
{
    ///
    /// @brief Constructs from a plain C-string error message.
    ///
    /// @param[in] m Description of the conversion failure.
    ///
    explicit bad_conversion(const char* m)
        : std::runtime_error(m) {}

    ///
    /// @brief Constructs from a std::string error message.
    ///
    /// @param[in] m Description of the conversion failure.
    ///
    explicit bad_conversion(const std::string& m)
        : std::runtime_error(m) {}
};

namespace detail {

// Attempts boost::lexical_cast<To>(v), rethrowing any failure as bad_conversion.
template <typename To, typename From>
inline static auto try_lexical_convert(From&& v) {
    try {
        return boost::lexical_cast<To>(v);
    }
    catch (const boost::bad_lexical_cast& e) {
        throw bad_conversion(e.what());
    }
}

// Default conversion strategy: delegates to try_lexical_convert.
template <typename From, typename To>
struct converter_traits
{
    inline static auto convert(From&& v) { return try_lexical_convert<To>(std::forward<From>(v)); }
};

// Specialisation avoiding lexical_cast overhead for a single character to string.
template <>
struct converter_traits<char, std::string>
{
    inline static auto convert(char v) { return std::string{v}; }
};

// Specialisation avoiding lexical_cast overhead for a C-string to string.
template <>
struct converter_traits<const char*, std::string>
{
    inline static auto convert(const char* v) { return std::string{v}; }
};

// Specialisation avoiding lexical_cast overhead for numeric types to string.
template <typename From>
struct converter_traits<From, std::enable_if<std::is_integral_v<From> || std::is_floating_point_v<From>, std::string>>
{
    inline static auto convert(From&& v) { return std::to_string(v); }
};

} // namespace detail

///
/// @brief Converts a value of type @p From to a value of type @p To.
///
/// Delegates to converter_traits, which uses boost::lexical_cast by default,
/// with specialisations avoiding unnecessary conversions for char, C-string,
/// and numeric-to-string cases.
///
/// @tparam To    The target type to convert to.
/// @tparam From  The source type to convert from.
/// @param[in] v  The value to convert.
/// @return The value of @p v converted to type @p To.
/// @throws bad_conversion if @p v cannot be converted to @p To.
///
template <typename To, typename From>
inline auto convert_to(From&& v) {
    using namespace ecf::detail;
    return detail::converter_traits<From, To>::convert(std::forward<From>(v));
}

} // namespace ecf

#endif /* ecflow_core_Converter_HPP */
