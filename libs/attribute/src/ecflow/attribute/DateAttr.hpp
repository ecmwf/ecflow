/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_attribute_DateAttr_HPP
#define ecflow_attribute_DateAttr_HPP

///
/// \note calendarChanged() once a Date is free, it stays free. It relies on parent cron/repeat to re-queue
///

#include <string>
#include <vector>

#include "ecflow/core/Chrono.hpp"

namespace cereal {
class access;
}

namespace ecf {
class Calendar;
} // namespace ecf

// Use default copy constructor, assignment operator, destructor
// Value of 0 for day,month,year means *, i.e. means any value
class DateAttr {
public:
    DateAttr(int day, int month, int year); // will throw std::out_of_range for if invalid date
    explicit DateAttr(const std::string&);  // will throw std::runtime_error for if invalid date
    DateAttr() = default;                   // for serialisation
    explicit DateAttr(const boost::gregorian::date& date)
        : day_(date.day()),
          month_(date.month()),
          year_(date.year()) {} // for test

    bool operator==(const DateAttr& rhs) const;
    bool operator<(const DateAttr& rhs) const;
    bool structureEquals(const DateAttr& rhs) const;

    void reset();
    void requeue();

    void setFree();   // ensures that isFree() always returns true
    void clearFree(); // resets the free flag
    bool isSetFree() const { return free_; }
    void calendarChanged(const ecf::Calendar& c, bool clear_at_midnight = true); // can set attribute free
    bool isFree(const ecf::Calendar&) const;
    bool checkForRequeue(const ecf::Calendar&) const;
    bool validForHybrid(const ecf::Calendar&) const;
    bool why(const ecf::Calendar&, std::string& theReasonWhy) const;

    // The state_change_no is never reset. Must be incremented if it can affect equality
    unsigned int state_change_no() const { return state_change_no_; }

    std::string name() const; // for display/gui only
    std::string toString() const;
    std::string dump() const;

    /// Check the date, will throw std::out_of_range or derivative if invalid
    static void checkDate(int day, int month, int year, bool allow_wild_cards);

    /// Extract the date, if return integer is zero, date was of the *, i.e. any day,month,year
    /// will throw std::runtime_error for parse errors
    /// expect:
    ///    15.11.2009
    ///    15.*.*
    ///    *.1.*
    static void getDate(const std::string& date, int& day, int& month, int& year);
    static DateAttr create(const std::string& dateString);
    static DateAttr create(const std::vector<std::string>& lineTokens, bool read_state);

    boost::gregorian::date next_matching_date(const ecf::Calendar&) const;

    // access
    int day() const { return day_; }
    int month() const { return month_; }
    int year() const { return year_; }

    bool is_free(const ecf::Calendar&) const; // ignores free_

public:
    void write(std::string&) const;

private:
    int day_{0};
    int month_{0};
    int year_{0};
    unsigned int state_change_no_{0}; // *not* persisted, only used on server side
    bool free_{false};                // persisted for use by why() on client side

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar);
};

#endif /* ecflow_attribute_DateAttr_HPP */
