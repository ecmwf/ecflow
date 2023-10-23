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

template <typename To>
struct converter_traits
{
    template <typename From>
    inline static auto convert(From&& v) {
        return try_lexical_convert<To>(std::forward<From>(v));
    }
};

template <>
struct converter_traits<std::string>
{
    template <typename From>
    inline static auto convert(From&& v) {

        if constexpr (std::is_same_v<From, char>) {
            return std::string{v};
        }
        else if constexpr (std::is_same_v<std::remove_cv_t<From>, const char*>) {
            return std::string(v);
        }
        else if constexpr (std::is_integral_v<From> || std::is_floating_point_v<From>) {
            return std::to_string(v);
        }

        return try_lexical_convert<std::string>(std::forward<From>(v));
    }
};

} // namespace details

template <typename To, typename From>
inline auto convert_to(From v) {
    using namespace ecf::details;
    return converter_traits<To>::convert(v);
}

} // namespace ecf

#endif
