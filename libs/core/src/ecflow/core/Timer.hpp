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

#include <array>
#include <chrono>
#include <functional>
#include <iostream>

///
/// @brief A simple elapsed-time utility that records the instant of its construction.
///
/// The elapsed time is measured from the moment the object is constructed. `Resolution`
/// controls the granularity of the reported duration.
///
/// @tparam Resolution The chrono duration type used to express elapsed time
///         (defaults to `std::chrono::milliseconds`).
///
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

    ///
    /// @brief Print the elapsed time since construction to standard output, prefixed by \p msg.
    ///
    /// @param msg The label printed before the elapsed-time value.
    ///
    void elapsed(const char* msg) const {
        const auto end = Clock::now();
        std::cout << msg << " " << std::chrono::duration_cast<Resolution>(end - start_).count() << std::endl;
    }

    ///
    /// @brief Return the elapsed time since construction.
    ///
    /// @return The elapsed duration cast to `Resolution`.
    ///
    Resolution elapsed() const { return std::chrono::duration_cast<Resolution>(Clock::now() - start_); }
};

namespace ecf {

///
/// @brief Measures the duration of a single function invocation.
///
/// @tparam Time The chrono duration type used to express the measured duration
///         (defaults to `std::chrono::microseconds`).
/// @tparam Clock The clock type used to take timestamps
///         (defaults to `std::chrono::high_resolution_clock`).
///
template <typename Time = std::chrono::microseconds, typename Clock = std::chrono::high_resolution_clock>
struct FunctionPerformanceTimer
{
    ///
    /// @brief Invoke \p f with \p args and return the elapsed time.
    ///
    /// @tparam F A callable type.
    /// @tparam Args Argument types forwarded to \p f.
    ///
    /// @param f The callable to invoke.
    /// @param args Arguments to forward to \p f.
    /// @return The elapsed duration cast to `Time`.
    ///
    template <typename F, typename... Args>
    static Time duration(F&& f, Args... args) {
        auto start = Clock::now();

        std::invoke(std::forward<F>(f), std::forward<Args>(args)...); // c++17

        auto end = Clock::now();
        return std::chrono::duration_cast<Time>(end - start);
    }
};

///
/// @brief A timer that measures elapsed wall-clock time with microsecond precision.
///
class DurationTimer {
public:
    using clock_t    = std::chrono::system_clock;
    using instant_t  = std::chrono::time_point<clock_t>;
    using duration_t = std::chrono::microseconds;

    DurationTimer()
        : start_time_(clock_t::now()) {}

    ~DurationTimer() = default;

    ///
    /// @brief Return the elapsed time since construction, in whole seconds.
    ///
    /// @return The elapsed duration, in whole seconds, since the start (i.e. creation of this DurationTimer).
    ///
    [[nodiscard]] int duration() const { return std::chrono::duration_cast<std::chrono::seconds>(elapsed()).count(); }

    ///
    /// @brief Return the elapsed time since construction.
    ///
    /// @return The elapsed duration since the start (i.e. creation of this DurationTimer).
    ///
    [[nodiscard]] duration_t elapsed() const {
        return std::chrono::duration_cast<duration_t>(clock_t::now() - start_time_);
    }

    ///
    /// @brief Return the elapsed time since construction, in decimal seconds.
    ///
    /// @return The elapsed duration since the start, in decimal seconds with microsecond precision.
    ///
    [[nodiscard]] double elapsed_seconds() const {
        auto us = std::chrono::duration_cast<std::chrono::microseconds>(elapsed());
        return static_cast<double>(us.count()) / 1000000.0;
    }

    ///
    /// @brief Return the elapsed time since construction, in decimal milliseconds.
    ///
    /// @return The elapsed duration since the start, in decimal milliseconds with microsecond precision.
    ///
    [[nodiscard]] double elapsed_milliseconds() const {
        auto us = std::chrono::duration_cast<std::chrono::microseconds>(elapsed());
        return static_cast<double>(us.count()) / 1000.0;
    }

    ///
    /// @brief Format a duration as a human-readable string.
    ///
    /// @param d The elapsed duration to format.
    /// @return The formatted string in the form `"HH:MM:SS.mmmuuu"`.
    ///
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

        std::array<char, 100> buffer;
        snprintf(buffer.data(),
                 buffer.size(),
                 "%02lld:%02lld:%02lld.%03lld%03lld",
                 static_cast<long long>(h.count()),
                 static_cast<long long>(m.count()),
                 static_cast<long long>(s.count()),
                 static_cast<long long>(ms.count()),
                 static_cast<long long>(us.count()));
        return std::string{buffer.data()};
    }

private:
    instant_t start_time_;
};

///
/// @brief A timer that reports the elapsed wall-clock time to standard output when it goes out of scope.
///
/// The elapsed time is printed to standard output when this object is destroyed (i.e. in the destructor).
///
class ScopedDurationTimer : public DurationTimer {
public:
    ///
    /// @brief Construct a scoped timer with the given label.
    ///
    /// @param msg The label printed alongside the elapsed time when the timer is destroyed.
    ///
    explicit ScopedDurationTimer(std::string msg)
        : DurationTimer(),
          msg_(std::move(msg)) {}

    ~ScopedDurationTimer();

private:
    std::string msg_;
};

///
/// @brief A snapshot of elapsed wall-clock, user-CPU, and system-CPU time.
///
struct PerformanceMeasure
{
    std::chrono::nanoseconds wall;   ///< Elapsed wall-clock time.
    std::chrono::nanoseconds user;   ///< Elapsed user-CPU time.
    std::chrono::nanoseconds system; ///< Elapsed system-CPU time.

    ///
    /// @brief Reset all durations to zero.
    ///
    void clear() {
        wall   = std::chrono::nanoseconds{0};
        user   = std::chrono::nanoseconds{0};
        system = std::chrono::nanoseconds{0};
    }

    ///
    /// @brief Capture the current wall-clock, user-CPU, and system-CPU times.
    ///
    /// @return A `PerformanceMeasure` snapshot representing the current instant.
    ///
    static PerformanceMeasure current();
};

///
/// @brief Compute the difference between two `PerformanceMeasure` snapshots.
///
/// @param lhs The later (end) snapshot.
/// @param rhs The earlier (start) snapshot.
/// @return A `PerformanceMeasure` whose fields hold the per-dimension elapsed time.
///
inline PerformanceMeasure operator-(const PerformanceMeasure& lhs, const PerformanceMeasure& rhs) {
    auto wall   = lhs.wall - rhs.wall;
    auto user   = lhs.user - rhs.user;
    auto system = lhs.system - rhs.system;
    return PerformanceMeasure{wall, user, system};
}

///
/// @brief A timer that measures wall-clock, user-CPU, and system-CPU time.
///
/// Call `start()` to reset the timer, and `elapsed()` to obtain the elapsed
/// `PerformanceMeasure` since the last reset (or construction).
///
class PerformanceTimer {
public:
    ///
    /// @brief Construct a performance timer, recording the start instant.
    ///
    PerformanceTimer()
        : times_{PerformanceMeasure::current()} {}

    ///
    /// @brief Reset the timer to the current instant.
    ///
    void start() noexcept { times_ = PerformanceMeasure::current(); }

    ///
    /// @brief Return the elapsed time since the last reset (or construction).
    ///
    /// @return A `PerformanceMeasure` snapshot of the elapsed times.
    ///
    PerformanceMeasure elapsed() const { return PerformanceMeasure::current() - times_; }

private:
    PerformanceMeasure times_;
};

///
/// @brief Write a human-readable performance summary of \p timer to \p os.
///
/// The output format is: `<wall>s wall, (<user>s user + <system>s system = <cpu>s) CPU (<percent>%)`
///
/// @param os The output stream to write to.
/// @param timer The timer whose elapsed performance to format.
/// @return A reference to \p os.
///
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

///
/// @brief A performance timer that reports elapsed time to standard output when it goes out of scope.
///
class ScopedPerformanceTimer : public PerformanceTimer {
public:
    ~ScopedPerformanceTimer() noexcept { std::cout << *this << std::endl; };
};

} // namespace ecf

#endif /* ecflow_core_Timer_HPP */
