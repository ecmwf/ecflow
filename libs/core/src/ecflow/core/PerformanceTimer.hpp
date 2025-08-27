/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_core_PerformanceTimer_HPP
#define ecflow_core_PerformanceTimer_HPP

#include <chrono>
#include <functional>
#include <iostream>

template <class Resolution = std::chrono::milliseconds>
class Timer {
public:
    using Clock = std::conditional_t<std::chrono::high_resolution_clock::is_steady,
                                     std::chrono::high_resolution_clock,
                                     std::chrono::steady_clock>;

private:
    const Clock::time_point start_ = Clock::now();

public:
    Timer()  = default;
    ~Timer() = default;
    void elapsed(const char* msg) const {
        const auto end = Clock::now();
        std::cout << msg << " " << std::chrono::duration_cast<Resolution>(end - start_).count() << std::endl;
    }

    Resolution elapsed() const { return std::chrono::duration_cast<Resolution>(Clock::now() - start_); }
};

namespace ecf {

/**
 * @brief A class to measure the duration of a function invocation
 *
 * @tparam Time the unit of time used to measure the duration of a function invocation
 * @tparam Clock the clock type used to measure the duration of a function invocation
 */
template <typename Time = std::chrono::microseconds, typename Clock = std::chrono::high_resolution_clock>
struct FunctionPerformanceTimer
{
    template <typename F, typename... Args>
    static Time duration(F&& f, Args... args) {
        auto start = Clock::now();

        std::invoke(std::forward<F>(f), std::forward<Args>(args)...); // c++17

        auto end = Clock::now();
        return std::chrono::duration_cast<Time>(end - start);
    }
};

struct PerformanceMeasure
{
    std::chrono::nanoseconds wall;
    std::chrono::nanoseconds user;
    std::chrono::nanoseconds system;

    void clear() {
        wall   = std::chrono::nanoseconds{0};
        user   = std::chrono::nanoseconds{0};
        system = std::chrono::nanoseconds{0};
    }

    static PerformanceMeasure current();
};

inline PerformanceMeasure operator-(const PerformanceMeasure& lhs, const PerformanceMeasure& rhs) {
    auto wall   = lhs.wall - rhs.wall;
    auto user   = lhs.user - rhs.user;
    auto system = lhs.system - rhs.system;
    return PerformanceMeasure{wall, user, system};
}

class PerformanceTimer {
public:
    PerformanceTimer() noexcept : times_{PerformanceMeasure::current()} {}

    void start() noexcept { times_ = PerformanceMeasure::current(); }
    PerformanceMeasure elapsed() const { return PerformanceMeasure::current() - times_; }

private:
    PerformanceMeasure times_;
};

inline std::ostream& operator<<(std::ostream& os, const PerformanceTimer& timer) {
    auto elapsed = timer.elapsed();
    auto w       = static_cast<double>(elapsed.wall.count()) / 1000000000.0;
    auto u       = static_cast<double>(elapsed.user.count()) / 1000000000.0;
    auto s       = static_cast<double>(elapsed.system.count()) / 1000000000.0;
    auto t       = u + s;
    auto p       = static_cast<double>(t) / static_cast<double>(w);
    os << w << "s wall, (" << u << "s user + " << s << "s system = " << t << "s) CPU (" << p << "%)";
    return os;
}

class ScopedPerformanceTimer : public PerformanceTimer {
public:
    ~ScopedPerformanceTimer() noexcept { std::cout << *this << std::endl; };
};

} // namespace ecf

#endif /* ecflow_core_PerformanceTimer_HPP */
