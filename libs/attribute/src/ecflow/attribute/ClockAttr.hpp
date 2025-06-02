/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_attribute_ClockAttr_HPP
#define ecflow_attribute_ClockAttr_HPP

#include <cstdint>

#include <boost/date_time/posix_time/posix_time_types.hpp>
namespace cereal {
class access;
}

namespace ecf {
class Calendar;
} // namespace ecf

/// The clock attribute is defined on the suite ONLY
/// Use default copy constructor and assignment operator, destructor
/// The clock attribute is used to initialise the calendar object
/// **********************************************************************
/// In the OLD sms the date is actually used as a gain factor(well at least
/// according to the user/reference manual), in the ec-flow
/// a date, is a date. i.e. it allows us to start a suite in the past.
///
/// The Constructor will update the State change number, since it can be added
/// by the AlterCmd. in the Client Context, state change number is not incremented
/// ************************************************************************
///
class ClockAttr {
public:
    /// The following constructor is used for test only. It allows us to
    /// create a clock attribute initialised with given date and time
    explicit ClockAttr(const boost::posix_time::ptime&, bool hybrid = false, bool positiveGain = true);
    ClockAttr(int day, int month, int year, bool hybrid = false);
    explicit ClockAttr(bool hybrid = false);

    void print(std::string&) const;
    bool operator==(const ClockAttr& rhs) const;

    void date(int day, int month, int year);
    void set_gain(int hour, int min, bool positiveGain = true);
    void set_gain_in_seconds(long theGain, bool positiveGain = true);

    void hybrid(bool f);
    void set_end_clock() { end_clock_ = true; }

    // clear local attributes so that, when the suite is requeued, the computer clock is synced
    void sync();

    void init_calendar(ecf::Calendar&);
    void begin_calendar(ecf::Calendar&) const;

    // The state_change_no is never reset. Must be incremented if it can affect equality
    unsigned int state_change_no() const { return state_change_no_; }

    // access
    int day() const { return day_; }
    int month() const { return month_; }
    int year() const { return year_; }
    long gain() const { return gain_; }
    bool positive_gain() const { return positiveGain_; }
    bool hybrid() const { return hybrid_; }
    std::string toString() const;
    boost::posix_time::ptime ptime() const;

public:
    void write(std::string& os) const;

private:
    long gain_{0}; // in seconds
    int day_{0};
    int month_{0};
    int year_{0};

    unsigned int state_change_no_; // *not* persisted, only used on server side
    bool hybrid_{false};
    bool positiveGain_{false};
    bool end_clock_{false}; // *NOT* persisted, used for end clock, simulator only

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const version);
};

#endif /* ecflow_attribute_ClockAttr_HPP */
