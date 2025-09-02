/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/attribute/ClockAttr.hpp"

#include "ecflow/attribute/DateAttr.hpp"
#include "ecflow/core/Calendar.hpp"
#include "ecflow/core/Converter.hpp"
#include "ecflow/core/Ecf.hpp"
#include "ecflow/core/Serialization.hpp"

using namespace ecf;

//==========================================================================================

ClockAttr::ClockAttr(const boost::posix_time::ptime& time, bool hybrid, bool positiveGain)
    : gain_(0),
      state_change_no_(Ecf::incr_state_change_no()),
      hybrid_(hybrid),
      positiveGain_(positiveGain) {
    auto theDate = time.date();
    day_         = theDate.day();
    month_       = theDate.month();
    year_        = theDate.year();

    tm t  = to_tm(time);
    gain_ = t.tm_hour * 3600 + t.tm_min * 60 + t.tm_sec;
}

ClockAttr::ClockAttr(int day, int month, int year, bool hybrid)
    : gain_(0),
      day_(day),
      month_(month),
      year_(year),
      state_change_no_(Ecf::incr_state_change_no()),
      hybrid_(hybrid) {
    // Will throw std::out_of_range exception
    DateAttr::checkDate(day, month, year, false /* disable wild cards */);
}

ClockAttr::ClockAttr(bool hybrid) : state_change_no_(Ecf::incr_state_change_no()), hybrid_(hybrid) {
}

std::string ClockAttr::toString() const {
    std::string ret;
    write(ret);
    return ret;
}

void ClockAttr::write(std::string& ss) const {
    if (!end_clock_) {
        ss += "clock ";
        if (hybrid_)
            ss += "hybrid ";
        else
            ss += "real ";
    }
    else {
        ss += "endclock ";
    }

    if (day_ != 0) {
        ss += ecf::convert_to<std::string>(day_);
        ss += ".";
        ss += ecf::convert_to<std::string>(month_);
        ss += ".";
        ss += ecf::convert_to<std::string>(year_);
        ss += " ";
    }

    if (gain_ != 0) {
        if (positiveGain_)
            ss += "+";
        ss += ecf::convert_to<std::string>(gain_);
    }
}

bool ClockAttr::operator==(const ClockAttr& rhs) const {
    if (hybrid_ != rhs.hybrid_) {
#ifdef DEBUG
        if (Ecf::debug_equality()) {
            std::cout << "hybrid_ != rhs.hybrid_\n";
        }
#endif
        return false;
    }

    if (day_ != rhs.day_) {
#ifdef DEBUG
        if (Ecf::debug_equality()) {
            std::cout << "day(" << day_ << ") != rhs.day(" << rhs.day_ << "\n";
        }
#endif
        return false;
    }

    if (month_ != rhs.month_) {
#ifdef DEBUG
        if (Ecf::debug_equality()) {
            std::cout << "month_(" << month_ << ") != rhs.month_(" << rhs.month_ << "\n";
        }
#endif
        return false;
    }

    if (year_ != rhs.year_) {
#ifdef DEBUG
        if (Ecf::debug_equality()) {
            std::cout << "year_(" << year_ << ") != rhs.year_(" << rhs.year_ << "\n";
        }
#endif
        return false;
    }

    if (gain_ != rhs.gain_) {
#ifdef DEBUG
        if (Ecf::debug_equality()) {
            std::cout << "gain_(" << gain_ << ") != rhs.gain_(" << rhs.gain_ << "\n";
        }
#endif
        return false;
    }

    if (positiveGain_ != rhs.positiveGain_) {
#ifdef DEBUG
        if (Ecf::debug_equality()) {
            std::cout << "positiveGain_(" << positiveGain_ << ") != rhs.positiveGain_(" << rhs.positiveGain_ << ")\n";
        }
#endif
        return false;
    }

    return true;
}

void ClockAttr::date(int day, int month, int year) {
    // Will throw std::out_of_range exception
    DateAttr::checkDate(day, month, year, false /* disable wild cards */);
    day_             = day;
    month_           = month;
    year_            = year;
    state_change_no_ = Ecf::incr_state_change_no();
}

void ClockAttr::set_gain(int hour, int min, bool positiveGain) {
    positiveGain_    = positiveGain;
    gain_            = (hour * 3600) + min * 60;
    state_change_no_ = Ecf::incr_state_change_no();
}

void ClockAttr::set_gain_in_seconds(long theGain, bool positiveGain) {
    positiveGain_    = positiveGain;
    gain_            = theGain;
    state_change_no_ = Ecf::incr_state_change_no();
}

void ClockAttr::hybrid(bool f) {
    hybrid_          = f;
    state_change_no_ = Ecf::incr_state_change_no();
}

void ClockAttr::sync() {
    // When begin_calendar() is called we will sync with computer clock.
    positiveGain_    = false;
    gain_            = 0;
    day_             = 0;
    month_           = 0;
    year_            = 0;
    state_change_no_ = Ecf::incr_state_change_no();
}

void ClockAttr::init_calendar(ecf::Calendar& calendar) {
    Calendar::Clock_t clockType = (hybrid_) ? Calendar::HYBRID : Calendar::REAL;
    calendar.init(clockType);
}

void ClockAttr::begin_calendar(ecf::Calendar& calendar) const {
    calendar.begin(ptime());
}

boost::posix_time::ptime ClockAttr::ptime() const {
    if (day_ != 0) {
        // Use the date given. ie we start from midnight on the given day + gain.
        auto theDate = boost::gregorian::date(year_, month_, day_);
        return {theDate, boost::posix_time::seconds(gain_)};
    }

    // Get the local time, second level resolution, based on the time zone settings of the computer.
    auto the_time = boost::posix_time::ptime(Calendar::second_clock_time());
    the_time += boost::posix_time::seconds(gain_);
    return the_time;
}

template <class Archive>
void ClockAttr::serialize(Archive& ar, std::uint32_t const version) {
    ar(CEREAL_NVP(hybrid_));
    CEREAL_OPTIONAL_NVP(ar, positiveGain_, [this]() { return positiveGain_; });
    CEREAL_OPTIONAL_NVP(ar, gain_, [this]() { return gain_ != 0; });
    CEREAL_OPTIONAL_NVP(ar, day_, [this]() { return day_ != 0; });
    CEREAL_OPTIONAL_NVP(ar, month_, [this]() { return month_ != 0; });
    CEREAL_OPTIONAL_NVP(ar, year_, [this]() { return year_ != 0; });
}
CEREAL_TEMPLATE_SPECIALIZE_V(ClockAttr);
