/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/core/TimeStamp.hpp"

#include <array>
#include <ctime>

namespace ecf {
namespace TimeStamp {

namespace {

///
/// @brief Format policy for the regular timestamp, including the year.
///
/// Format: `"[HH:MM:SS DD.MM.YYYY] "` (strftime pattern: `"[%H:%M:%S %d.%m.%Y] "`).
///
struct regular
{
    /// strftime format string.
    static constexpr char const* format = "[%H:%M:%S %d.%m.%Y] ";
    /// Buffer size in bytes, including the null terminator.
    static constexpr size_t size = 23;
};

///
/// @brief Format policy for the brief timestamp, omitting the year.
///
/// Format: `"[HH:MM:SS DD.MM] "` (strftime pattern: `"[%H:%M:%S %d.%m] "`).
///
struct brief
{
    /// strftime format string.
    static constexpr char const* format = "[%H:%M:%S %d.%m] ";
    /// Buffer size in bytes, including the null terminator.
    static constexpr size_t size = 18;
};

///
/// @brief Format the current local time as a timestamp string.
///
/// @tparam FMT A format policy providing a strftime-compatible `format` string and the
///         corresponding `size` (in bytes, including the null terminator). Defaults to `regular`.
///
/// @return The current local time formatted according to `FMT::format`.
///
template <typename FMT = regular>
std::string format_now() {
    std::time_t now = std::time(nullptr);
    std::array<char, FMT::size> buffer;
    std::strftime(buffer.data(), buffer.size(), FMT::format, std::localtime(&now));
    return std::string{buffer.data()};
}

} // namespace

std::string now() {
    return format_now();
}

void now(std::string& buffer) {
    buffer = format_now();
}

void now_in_brief(std::string& buffer) {
    buffer = format_now<brief>();
}

} // namespace TimeStamp
} // namespace ecf
