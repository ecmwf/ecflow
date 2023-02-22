//============================================================================
// Name        : TimeStamp
// Author      : Avi
// Revision    : $Revision: #57 $
//
// Copyright 2009- ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================
#include "TimeStamp.hpp"

#include <ctime>

namespace ecf {
namespace TimeStamp {

namespace {

struct regular
{
    static constexpr char const* format = "[%H:%M:%S %-e.%-m.%Y] ";
    static constexpr size_t size        = 23;
};

struct brief
{
    static constexpr char const* format = "[%H:%M:%S %-e.%-m] ";
    static constexpr size_t size        = 18;
};

template <typename FMT = regular>
std::string format_now() {
    std::time_t now = std::time(nullptr);
    char buffer[FMT::size];
    std::strftime(buffer, sizeof(buffer), FMT::format, std::localtime(&now));
    return buffer;
}

} // namespace

std::string now() {
    return format_now();
}

void now(std::string& time_stamp) {
    time_stamp = format_now();
}

void now_in_brief(std::string& time_stamp) {
    time_stamp = format_now<brief>();
}

} // namespace TimeStamp
} // namespace ecf
