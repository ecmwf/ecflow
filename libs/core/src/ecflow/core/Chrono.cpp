// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include "Chrono.hpp"

#include <algorithm>
#include <array>
#include <charconv>
#include <iomanip>
#include <limits>
#include <sstream>
#include <string_view>
#include <vector>

#include "ecflow/core/Message.hpp"
#include "ecflow/core/Serialization.hpp"

namespace ecf {

namespace {

template <typename C, typename T>
bool contains(const C& collection, const T& value) {
    return std::find(std::begin(collection), std::end(collection), value) != std::end(collection);
}

int days_of_month(int month /* [0-11] */, int year) {
    if (contains(std::array{0, 2, 4, 6, 7, 9, 11}, month)) {
        return 31;
    }
    if (contains(std::array{3, 5, 8, 10}, month)) {
        return 30;
    }
    if (month == 1) {
        return (year % 4 == 0 && year % 100) || year % 400 == 0 ? 29 : 28;
    }
    throw std::runtime_error("Invalid month detected, while calculating the number of days");
}

bool is_valid_days_of_month(int day /* [1-31] */, int month /* [0-11] */, int year) {
    return ((1 <= day) && (day <= days_of_month(month, year)));
}

} // namespace

Instant Instant::parse(const std::string& value) {
    std::tm tm = {};
    std::istringstream ss(value);
    ss >> std::get_time(&tm, "%Y%m%dT%H%M%S");
    // Validate if parsing was successful
    if (ss.fail()) {
        throw std::runtime_error("Unable to parse invalid instant value: " + value);
    }
    // Extra validation of the parsed values (n.b. parsing does not ensure number of days is in agreement with the
    // month)
    if (!is_valid_days_of_month(tm.tm_mday, tm.tm_mon, tm.tm_year + 1900)) {
        throw std::runtime_error("Detected invalid number of days for instant value: " + value);
    }

    // Unfortunately, std::mktime does the conversion considering locale, and thus considering timezones e.g. BST.
    // The following convert std::tm to time_t (i.e. seconds since Posix epoch disregarding the timezone,
    // adapted from Howard Hinnant's algorithms: http://howardhinnant.github.io/date_algorithms.html (days_from_civil).
    //
    // Note: All this can eventually be replaced by using C++20's std::chrono::parse.
    std::time_t time;
    {
        int y      = tm.tm_year + 1900;
        unsigned m = tm.tm_mon + 1;
        unsigned d = tm.tm_mday;
        y -= m <= 2;
        const int era      = (y >= 0 ? y : y - 399) / 400;
        const unsigned yoe = static_cast<unsigned>(y - era * 400);           // [0, 399]
        const unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1; // [0, 365]
        const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;          // [0, 146096]

        time = (era * 146097 + static_cast<int>(doe) - 719468) * 86400 + tm.tm_hour * 3600 + tm.tm_min * 60 + tm.tm_sec;
    }

    return Instant{Instant::clock_t::from_time_t(time)};
}

std::string Instant::format(const Instant& value) {
    auto tt    = std::chrono::system_clock::to_time_t(value.instant_);
    std::tm tm = *std::gmtime(&tt);

    return MESSAGE(std::put_time(&tm, "%Y%m%dT%H%M%S"));
}

std::ostream& operator<<(std::ostream& o, const Instant& v) {
    o << Instant::format(v);
    return o;
}

bool operator==(const Instant& rhs, const Instant& lhs) {
    return rhs.instant_ == lhs.instant_;
}

bool operator!=(const Instant& rhs, const Instant& lhs) {
    return !(rhs == lhs);
}

bool operator<(const Instant& rhs, const Instant& lhs) {
    return rhs.instant_ < lhs.instant_;
}

bool operator<=(const Instant& rhs, const Instant& lhs) {
    return rhs == lhs || rhs < lhs;
}

bool operator>(const Instant& rhs, const Instant& lhs) {
    return !(rhs <= lhs);
}

bool operator>=(const Instant& rhs, const Instant& lhs) {
    return !(rhs < lhs);
}

Instant operator+(const Instant& rhs, const Duration& lhs) {
    return Instant{rhs.instant_ + lhs.as_seconds()};
}

Instant operator-(const Instant& rhs, const Duration& lhs) {
    return Instant{rhs.instant_ - lhs.as_seconds()};
}

Duration operator-(const Instant& rhs, const Instant& lhs) {
    auto diff = rhs.instant_ - lhs.instant_;
    return Duration{std::chrono::duration_cast<std::chrono::seconds>(diff)};
}

Instant coerce_from_seconds_into_instant(long value) {
    auto seconds_since_reference = std::chrono::seconds{value};
    auto instant                 = Instant{Instant::instant_t{seconds_since_reference}};
    return instant;
}

long coerce_from_instant_into_seconds(const Instant& value) {
    return coerce_from_instant_into<std::chrono::seconds>(value);
}

namespace detail {

template <class DurationIn, class FirstDuration, class... RestDurations>
std::string format_duration(DurationIn d) {
    auto value = std::chrono::duration_cast<FirstDuration>(d);

    std::ostringstream ss;
    ss << std::setw(2) << std::setfill('0') << value.count(); // Ensure left 0-padded value with 2 digits
    std::string out = ss.str();

    if constexpr (sizeof...(RestDurations) > 0) {
        // Important!
        // The whole duration has the same sign as the first component
        // e.g. -23:59:59, is interpreted as (-23 hours) + (-59 minutes) + (-59 seconds) = -86399 seconds
        int sign = value >= FirstDuration{0} ? 1 : -1;
        out += ":" + format_duration<DurationIn, RestDurations...>(sign * (d - value));
    }

    return out;
}

} // namespace detail

Duration Duration::parse(const std::string& value) {
    auto invalid = [&value](const std::string& reason) {
        return std::runtime_error("Unable to parse invalid duration value: " + value + " (" + reason + ")");
    };

    // Split the value into hours, minutes and seconds
    std::vector<std::string_view> fields;
    std::string_view rest = value;
    for (auto found = rest.find(':');; found = rest.find(':')) {
        fields.push_back(rest.substr(0, found));
        if (found == std::string_view::npos) {
            break;
        }
        rest.remove_prefix(found + 1);
    }
    if (fields.size() > 3) {
        throw invalid("expected at most hours, minutes and seconds");
    }

    using count_t                                 = Duration::duration_t::rep;
    constexpr std::array<count_t, 3> field_length = {3600, 60, 1};
    constexpr count_t largest                     = std::numeric_limits<count_t>::max();

    bool negative = false;
    bool has_sign = false;
    count_t total = 0;
    for (std::size_t i = 0; i < fields.size(); ++i) {
        auto field = fields[i];

        // A sign applies to the whole duration. It is accepted at the start of the value (e.g. -00:30:00), and,
        // as written by Duration::format, on the first non-zero field after zero fields (e.g. 00:-30:00)
        if (!field.empty() && (field.front() == '+' || field.front() == '-')) {
            bool leading           = (i == 0);
            bool written_by_format = !has_sign && field.front() == '-' && total == 0;
            if (!leading && !written_by_format) {
                throw invalid("misplaced sign");
            }
            negative = (field.front() == '-');
            has_sign = true;
            field.remove_prefix(1);
        }

        if (field.empty() || !std::all_of(field.begin(), field.end(), [](char c) { return c >= '0' && c <= '9'; })) {
            throw invalid("expected [+|-]H[:MM[:SS]]");
        }

        count_t number = 0;
        if (auto [_, error] = std::from_chars(field.data(), field.data() + field.size(), number);
            error != std::errc{}) {
            throw invalid("out of range");
        }
        if (i > 0 && number > 59) {
            throw invalid("minutes and seconds must be less than 60");
        }
        if (number > (largest - total) / field_length[i]) {
            throw invalid("out of range");
        }
        total += number * field_length[i];
    }

    return Duration{std::chrono::seconds{negative ? -total : total}};
}

std::string Duration::format(const Duration& value) {
    return detail::
        format_duration<Duration::duration_t, std::chrono::hours, std::chrono::minutes, std::chrono::seconds>(
            value.duration_);
}

std::ostream& operator<<(std::ostream& o, const Duration& v) {
    o << Duration::format(v);
    return o;
}

bool operator==(const Duration& rhs, const Duration& lhs) {
    return rhs.duration_ == lhs.duration_;
}

bool operator!=(const Duration& rhs, const Duration& lhs) {
    return !(rhs == lhs);
}

bool operator<(const Duration& rhs, const Duration& lhs) {
    return rhs.duration_ < lhs.duration_;
}

bool operator<=(const Duration& rhs, const Duration& lhs) {
    return rhs == lhs || rhs < lhs;
}

bool operator>(const Duration& rhs, const Duration& lhs) {
    return !(rhs <= lhs);
}

bool operator>=(const Duration& rhs, const Duration& lhs) {
    return !(rhs < lhs);
}

template <class Archive>
void Instant::serialize(Archive& ar, std::uint32_t const version) {
    ar(instant_);
}

template <class Archive>
void Duration::serialize(Archive& ar, std::uint32_t const version) {
    ar(duration_);
}

CEREAL_TEMPLATE_SPECIALIZE_V(Instant);
CEREAL_TEMPLATE_SPECIALIZE_V(Duration);

} // namespace ecf
