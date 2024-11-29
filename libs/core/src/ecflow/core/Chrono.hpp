/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_core_Chrono_HPP
#define ecflow_core_Chrono_HPP

#include <chrono>
#include <string>

namespace cereal {
class access;
}

namespace ecf {

class Instant;
class Duration;

Instant coerce_to_instant(long value);
long coerce_from_instant(const Instant& value);

class Instant {
public:
    using clock_t   = std::chrono::system_clock;
    using instant_t = std::chrono::time_point<Instant::clock_t>;

    Instant() : instant_() {}
    explicit Instant(instant_t instant)
        : instant_(std::chrono::duration_cast<std::chrono::seconds>(instant.time_since_epoch())) {}
    explicit Instant(std::chrono::seconds seconds_since_reference) : instant_(seconds_since_reference) {}

    static Instant parse(const std::string& value);
    static std::string format(const Instant& value);

    friend bool operator==(const Instant& rhs, const Instant& lhs);
    friend bool operator<(const Instant& rhs, const Instant& lhs);

    friend Instant operator+(const Instant& rhs, const Duration& lhs);
    friend Instant operator-(const Instant& rhs, const Duration& lhs);
    friend Duration operator-(const Instant& rhs, const Instant& lhs);

    friend Instant coerce_to_instant(long value);
    friend long coerce_from_instant(const Instant& value);

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

    Duration() : duration_{} {}
    template <typename DURATION>
    explicit Duration(DURATION duration) : duration_{std::chrono::duration_cast<duration_t>(duration)} {}

    [[nodiscard]] duration_t as_seconds() const { return duration_; }

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

#endif // ECFLOW_CHRONO_HPP
