// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <chrono>
#include <string>

#include <boost/date_time/gregorian/gregorian_types.hpp>
#include <boost/date_time/local_time/conversion.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/date_time/posix_time/time_formatters.hpp>
#include <boost/date_time/posix_time/time_parsers.hpp>

namespace cereal {
class access;
}

namespace ecf {

class Instant;
class Duration;

Instant coerce_from_seconds_into_instant(long value);
long coerce_from_instant_into_seconds(const Instant& value);

template <typename DURATION = std::chrono::seconds>
auto coerce_from_instant_into(const Instant& value);

class Instant {
public:
    using clock_t   = std::chrono::system_clock;
    using instant_t = std::chrono::time_point<Instant::clock_t>;

    Instant()
        : instant_() {}
    explicit Instant(instant_t instant)
        : instant_(std::chrono::duration_cast<std::chrono::seconds>(instant.time_since_epoch())) {}
    explicit Instant(std::chrono::seconds seconds_since_reference)
        : instant_(seconds_since_reference) {}

    static Instant parse(const std::string& value);
    static std::string format(const Instant& value);

    friend bool operator==(const Instant& rhs, const Instant& lhs);
    friend bool operator<(const Instant& rhs, const Instant& lhs);

    friend Instant operator+(const Instant& rhs, const Duration& lhs);
    friend Instant operator-(const Instant& rhs, const Duration& lhs);
    friend Duration operator-(const Instant& rhs, const Instant& lhs);

    friend Instant coerce_from_seconds_into_instant(long value);
    friend long coerce_from_instant_into_seconds(const Instant& value);

    template <typename DURATION>
    friend auto coerce_from_instant_into(const Instant& value) {
        return std::chrono::duration_cast<DURATION>(value.instant_.time_since_epoch()).count();
    }

    const instant_t& tp() const { return instant_; }

private:
    instant_t instant_;

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const version);
};

std::ostream& operator<<(std::ostream& o, const Instant& v);

bool operator==(const Instant& rhs, const Instant& lhs);
bool operator!=(const Instant& rhs, const Instant& lhs);
bool operator<(const Instant& rhs, const Instant& lhs);
bool operator<=(const Instant& rhs, const Instant& lhs);
bool operator>(const Instant& rhs, const Instant& lhs);
bool operator>=(const Instant& rhs, const Instant& lhs);

class Duration {
public:
    using duration_t = std::chrono::seconds;

    Duration()
        : duration_{} {}
    template <typename DURATION>
    explicit Duration(DURATION duration)
        : duration_{std::chrono::duration_cast<duration_t>(duration)} {}

    [[nodiscard]] duration_t as_seconds() const { return duration_; }

    ///
    /// @brief Parses a duration given as [+|-]H[:MM[:SS]].
    ///
    /// The hours are any number of digits; the minutes and seconds are less than 60. A sign applies to the whole
    /// duration, and is accepted either:
    ///
    ///   - at the start of the value (e.g. -00:30:00 is minus 30 minutes); or
    ///   - on the first non-zero field after zero fields (e.g. 00:-30:00), as written by format().
    ///
    /// @param[in] value The text to parse.
    /// @return The parsed duration.
    /// @throws std::runtime_error if the value is empty, has an empty or non-numeric field, more than three fields,
    ///         minutes or seconds of 60 or more, a sign anywhere else, or exceeds the range of seconds.
    ///
    static Duration parse(const std::string& value);
    static std::string format(const Duration& value);

    friend bool operator==(const Duration& rhs, const Duration& lhs);
    friend bool operator<(const Duration& rhs, const Duration& lhs);

private:
    duration_t duration_;

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const version);
};

std::ostream& operator<<(std::ostream& o, const Duration& v);

bool operator==(const Duration& rhs, const Duration& lhs);
bool operator!=(const Duration& rhs, const Duration& lhs);
bool operator<(const Duration& rhs, const Duration& lhs);
bool operator<=(const Duration& rhs, const Duration& lhs);
bool operator>(const Duration& rhs, const Duration& lhs);
bool operator>=(const Duration& rhs, const Duration& lhs);

} // namespace ecf
