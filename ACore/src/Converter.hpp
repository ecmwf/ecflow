/*
 * Copyright 2023- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ECFLOW_CORE_CONVERTER_HPP
#define ECFLOW_CORE_CONVERTER_HPP

#include <string>
#include <utility>

#include <boost/lexical_cast.hpp>

namespace ecf {

struct bad_conversion : public std::runtime_error
{
    explicit bad_conversion(const char* m) : std::runtime_error(m) {}
    explicit bad_conversion(const std::string& m) : std::runtime_error(m) {}
};

namespace details {

template <typename To, typename From>
inline static auto try_lexical_convert(From&& v) {
    try {
        return boost::lexical_cast<To>(v);
    }
    catch (const boost::bad_lexical_cast& e) {
        throw bad_conversion(e.what());
    }
}

}

template <typename From, typename To>
struct converter_traits
{
    inline static auto convert(From&& v) {
        return details::try_lexical_convert<To>(std::forward<From>(v));
    }
};

template <>
struct converter_traits<char, std::string>
{
    inline static auto convert(char v) { return std::string{v}; }
};

template <>
struct converter_traits<const char*, std::string>
{
    inline static auto convert(const char* v) { return std::string{v}; }
};

template <typename From>
struct converter_traits<From, std::enable_if<std::is_integral_v<From> || std::is_floating_point_v<From>,std::string>>
{
    inline static auto convert(From&& v) { return std::to_string(v); }
};

template <typename To, typename From>
inline auto convert_to(From&& v) {
    using namespace ecf::details;
    return converter_traits<From, To>::convert(std::forward<From>(v));
}

} // namespace ecf

#endif
