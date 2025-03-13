/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_attribute_RepeatRange_HPP
#define ecflow_attribute_RepeatRange_HPP

#include "ecflow/attribute/RepeatAttr.hpp"
#include "ecflow/core/Calendar.hpp"

namespace ecf {

template <typename R>
struct Range
{
    explicit Range(const R& r) : r_(r) {}
    const R& r_;
};

template <>
struct Range<RepeatDay>
{
    using size_type  = std::size_t;
    using iterator   = std::size_t;
    using value_type = int;

    explicit Range(const RepeatDay& r) : r_(r) {}

    iterator begin() const { return 0; }
    iterator end() const { return 0; }
    iterator current_index() const { return r_.index_or_value(); }

    value_type current_value() const { return r_.value(); }

    size_type size() const { return end() - begin(); }

private:
    const RepeatDay& r_;
};

template <>
struct Range<RepeatDate>
{
    using size_type  = std::size_t;
    using iterator   = std::size_t;
    using value_type = int;

    explicit Range(const RepeatDate& r) : r_(r) {}

    iterator begin() const { return 0; }
    iterator end() const {
        int i = ecf::CalendarDate(r_.start()).as_julian_day().value();
        int j = ecf::CalendarDate(r_.end()).as_julian_day().value();
        int s = r_.step();
        int d = (j - i) + 1;
        return (d / s) + ((d % s == 0) ? 0 : 1);
    }
    iterator current_index() const {
        auto i   = ecf::CalendarDate(r_.start()).as_julian_day().value();
        auto s   = r_.step();
        auto v   = ecf::CalendarDate(r_.value()).as_julian_day().value();
        auto idx = (v - i) / s;
        return idx;
    }

    value_type current_value() const { return r_.value(); }

    value_type at(iterator i) const {
        int j = ecf::CalendarDate(r_.start()).as_julian_day().value();
        return ecf::JulianDay(j + i * r_.step()).as_calendar_date().value();
    }

    size_type size() const { return end() - begin(); }

private:
    const RepeatDate& r_;
};

template <>
struct Range<RepeatDateList>
{
    using date_t     = int;
    using size_type  = std::size_t;
    using iterator   = std::size_t;
    using value_type = date_t;

    explicit Range(const RepeatDateList& r) : r_(r) {}

    iterator begin() const { return 0; }
    iterator end() const { return r_.indexNum(); }
    iterator current_index() const { return r_.index_or_value(); }

    value_type current_value() const { return r_.value(); }

    value_type at(iterator i) const { return r_.value_at(i); }

    size_type size() const { return end() - begin(); }

private:
    const RepeatDateList& r_;
};

template <>
struct Range<RepeatDateTime>
{
    using size_type  = std::size_t;
    using iterator   = std::size_t;
    using value_type = Instant;

    explicit Range(const RepeatDateTime& r) : r_(r) {}

    iterator begin() const { return 0; }
    iterator end() const {
        auto i = r_.start_instant();
        auto j = r_.end_instant();
        auto s = r_.step_duration();
        auto d = (j - i);

        auto idx  = d.as_seconds().count() / s.as_seconds().count();
        auto last = i + Duration{std::chrono::seconds{s.as_seconds().count() * idx}};
        if (last <= j) {
            idx++;
        }
        return idx;
    }
    iterator current_index() const {
        auto i   = r_.start_instant();
        auto s   = r_.step_duration();
        auto v   = r_.value_instant();
        auto idx = (v - i).as_seconds().count() / s.as_seconds().count();
        return idx;
    }

    value_type current_value() const { return r_.value_instant(); }

    value_type at(iterator i) const {
        auto j = r_.start_instant();
        auto s = r_.step_duration();
        return j + Duration{std::chrono::seconds{s.as_seconds().count() * i}};
    }

    size_type size() const { return end() - begin(); }

private:
    const RepeatDateTime& r_;
};

template <>
struct Range<RepeatInteger>
{
    using size_type  = std::size_t;
    using iterator   = std::size_t;
    using value_type = int;

    explicit Range(const RepeatInteger& r) : r_(r) {}

    iterator begin() const { return 0; }
    iterator end() const {
        int i    = r_.start();
        int j    = r_.end();
        int s    = r_.step();
        int d    = (j - i) + 1;
        auto idx = (d / s) + ((d % s == 0) ? 0 : 1);
        return idx;
    }
    iterator current_index() const {
        auto i   = r_.start();
        auto s   = r_.step();
        auto v   = r_.value();
        auto idx = (v - i) / s;
        return idx;
    }

    value_type current_value() const { return r_.value(); }

    value_type at(iterator i) const { return r_.start() + i * r_.step(); }

    size_type size() const { return end() - begin(); }

private:
    const RepeatInteger& r_;
};

template <>
struct Range<RepeatEnumerated>
{
    using size_type  = std::size_t;
    using iterator   = std::size_t;
    using value_type = std::string;

    explicit Range(const RepeatEnumerated& r) : r_(r) {}

    iterator begin() const { return 0; }
    iterator end() const { return r_.end() + 1; }
    iterator current_index() const { return r_.index_or_value(); }

    value_type current_value() const { return r_.valueAsString(); }

    value_type at(iterator i) const { return r_.value_as_string(i); }

    size_type size() const { return end() - begin(); }

private:
    const RepeatEnumerated& r_;
};

template <>
struct Range<RepeatString>
{
    using size_type  = std::size_t;
    using iterator   = std::size_t;
    using value_type = std::string;

    explicit Range(const RepeatString& r) : r_(r) {}

    iterator begin() const { return 0; }
    iterator end() const { return r_.end() + 1; }
    iterator current_index() const { return r_.index_or_value(); }

    value_type current_value() const { return r_.valueAsString(); }

    value_type at(iterator i) const { return r_.value_as_string(i); }

    size_type size() const { return end() - begin(); }

private:
    const RepeatString& r_;
};

template <typename T>
auto front(const Range<T>& rng) {
    assert(rng.size() > 0);
    return rng.at(rng.begin());
}

template <typename T>
auto back(const Range<T>& rng) {
    assert(rng.size() > 0);
    return rng.at(rng.end() - 1);
}

template <typename T>
auto size(const Range<T>& rng) {
    return rng.size();
}

template <typename T>
bool empty(const Range<T>& rng) {
    return size(rng) == 0;
}

struct Limits
{
    size_t begin;
    size_t end;
    size_t current;
};

Limits limits_of(const RepeatBase* repeat) {
    if (auto r1 = dynamic_cast<const RepeatDay*>(repeat)) {
        Range<RepeatDay> rng(*r1);
        return {rng.begin(), rng.end(), rng.current_index()};
    }
    else if (auto r2 = dynamic_cast<const RepeatDate*>(repeat)) {
        Range<RepeatDate> rng(*r2);
        return {rng.begin(), rng.end(), rng.current_index()};
    }
    else if (auto r3 = dynamic_cast<const RepeatDateList*>(repeat)) {
        Range<RepeatDateList> rng(*r3);
        return {rng.begin(), rng.end(), rng.current_index()};
    }
    else if (auto r4 = dynamic_cast<const RepeatDateTime*>(repeat)) {
        Range<RepeatDateTime> rng(*r4);
        return {rng.begin(), rng.end(), rng.current_index()};
    }
    else if (auto r5 = dynamic_cast<const RepeatInteger*>(repeat)) {
        Range<RepeatInteger> rng(*r5);
        return {rng.begin(), rng.end(), rng.current_index()};
    }
    else if (auto r6 = dynamic_cast<const RepeatEnumerated*>(repeat)) {
        Range<RepeatEnumerated> rng(*r6);
        return {rng.begin(), rng.end(), rng.current_index()};
    }
    else if (auto r7 = dynamic_cast<const RepeatString*>(repeat)) {
        Range<RepeatString> rng(*r7);
        return {rng.begin(), rng.end(), rng.current_index()};
    }
    assert(false); // Should never be reached!
    return {0, 0, 0};
}

template <typename T>
auto make_range(const Repeat& repeat) {
    return Range{repeat.as<const T&>()};
}

} // namespace ecf

#endif /* ecflow_attribute_RepeatRange_HPP */
