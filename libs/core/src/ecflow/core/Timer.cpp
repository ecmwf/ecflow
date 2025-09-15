/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/core/Timer.hpp"

#include <unistd.h>

#include <sys/times.h>

namespace ecf {

ScopedDurationTimer::~ScopedDurationTimer() {
    std::cout << msg_ << " " << elapsed_milliseconds() / 1000.0 << "s\n";
}

PerformanceMeasure PerformanceMeasure::current() {

    // Retrieve the number of tick per second used by this specific machine
    //
    // Note: How to achieve this is explained in the documentation of "time.h" / time types.
    //
    int64_t ticks_per_second = ::sysconf(_SC_CLK_TCK);
    {
        if (ticks_per_second <= 0) {
            throw std::runtime_error("unable to retrieve clock frequency information");
        }

        ticks_per_second = 1000000000 / ticks_per_second;
        if (ticks_per_second == 0) {
            throw std::runtime_error("unable to calculate clock frequency");
        }
    }

    ::tms tm;
    clock_t c = ::times(&tm);

    if (static_cast<clock_t>(-1) == c) {
        throw std::runtime_error("unable to retrieve process times");
    }

    auto wall   = std::chrono::nanoseconds(c) * ticks_per_second;
    auto system = std::chrono::nanoseconds(tm.tms_stime + tm.tms_cstime) * ticks_per_second;
    auto user   = std::chrono::nanoseconds(tm.tms_utime + tm.tms_cutime) * ticks_per_second;

    return {wall, system, user};
}

} // namespace ecf
