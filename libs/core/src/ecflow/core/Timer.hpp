/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_core_Timer_HPP
#define ecflow_core_Timer_HPP

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

/**
 * @brief A timer capable of measuring and reporting the elapsed wall clock time in microseconds.
 */
class DurationTimer {
public:
    using clock_t    = std::chrono::system_clock;
    using instant_t  = std::chrono::time_point<clock_t>;
    using duration_t = std::chrono::microseconds;

    DurationTimer() : start_time_(clock_t::now()) {}

    ~DurationTimer() = default;

    /**
     * @return the elapsed duration, in whole seconds, since the start (i.e. creation of this DurationTimer)
     */
    [[nodiscard]] int duration() const { return std::chrono::duration_cast<std::chrono::seconds>(elapsed()).count(); }

    /**
     * @return the elapsed duration, since the start (i.e. creation of this DurationTimer)
     */
    [[nodiscard]] duration_t elapsed() const {
        return std::chrono::duration_cast<duration_t>(clock_t::now() - start_time_);
    }

    /**
     * @return the elapsed duration since the start, in decimal seconds with microsecond precision
     */
    [[nodiscard]] double elapsed_seconds() const {
        auto us = std::chrono::duration_cast<std::chrono::microseconds>(elapsed());
        return static_cast<double>(us.count()) / 1000000.0;
    }

    /**
     * @return the elapsed duration since the start, in decimal milliseconds with microsecond precision
     */
    [[nodiscard]] double elapsed_milliseconds() const {
        auto us = std::chrono::duration_cast<std::chrono::microseconds>(elapsed());
        return static_cast<double>(us.count()) / 1000.0;
    }

    /**
     * Formats the given duration as a string
     *
     * @param d the elapsed duration to format
     * @return the formatted string in the form "HH:MM:SS.mmmuuu"
     */
    static std::string to_simple_string(duration_t d) {
        using namespace std::chrono;
        auto h = duration_cast<hours>(d);
        d -= h;
        auto m = duration_cast<minutes>(d);
        d -= m;
        auto s = duration_cast<seconds>(d);
        d -= s;
        auto ms = duration_cast<milliseconds>(d);
        d -= ms;
        auto us = duration_cast<microseconds>(d);

        char buffer[100];
        snprintf(buffer,
                 sizeof(buffer),
                 "%02lld:%02lld:%02lld.%03lld%03lld",
                 static_cast<long long>(h.count()),
                 static_cast<long long>(m.count()),
                 static_cast<long long>(s.count()),
                 static_cast<long long>(ms.count()),
                 static_cast<long long>(us.count()));
        return std::string(buffer);
    }

private:
    instant_t start_time_;
};

/**
 * @brief A timer capable of measuring and reporting the elapsed wall clock time in microseconds.
 *
 * The elapsed time is reported to standard out when this object goes out of scope (i.e. in the dtor).
 */
class ScopedDurationTimer : public DurationTimer {
public:
    explicit ScopedDurationTimer(std::string msg) : DurationTimer(), msg_(std::move(msg)) {}

    ~ScopedDurationTimer();

private:
    std::string msg_;
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

#endif /* ecflow_core_Timer_HPP */
