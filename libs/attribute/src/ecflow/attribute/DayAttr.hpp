/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_attribute_DayAttr_HPP
#define ecflow_attribute_DayAttr_HPP

#include <vector>

#include <boost/date_time/posix_time/posix_time_types.hpp>
namespace cereal {
class access;
}

namespace ecf {
class Calendar;
} // namespace ecf

// Use default copy constructor, assignment operator, destructor
class DayAttr {
public:
    enum Day_t { SUNDAY = 0, MONDAY = 1, TUESDAY = 2, WEDNESDAY = 3, THURSDAY = 4, FRIDAY = 5, SATURDAY = 6 };
    DayAttr() = default;
    explicit DayAttr(Day_t day) : day_(day) {}
    explicit DayAttr(const std::string& str) : day_(DayAttr::getDay(str)) {}
    explicit DayAttr(const boost::gregorian::date& date)
        : day_(static_cast<DayAttr::Day_t>(date.day_of_week().as_number())),
          date_(date) {}

    void print(std::string&) const;
    bool operator==(const DayAttr& rhs) const;
    bool operator<(const DayAttr& rhs) const { return day_ < rhs.day_; }
    bool structureEquals(const DayAttr& rhs) const;

    /// called when definition is restored from a file/checkpoint. handle reset of date_
    void handle_migration(const ecf::Calendar&);

    // called at begin time.
    void reset();
    void reset(const ecf::Calendar& c);

    // called when we need a requeue based on a time attribute. Should *NOT* clear expired flag.
    void requeue_time();

    // called when re-queueing because of:
    //    - automatic re-queue due to repeat increment
    //    - manual re-queue
    // Clears expired flag, and sets day attribute to a next matching *FUTURE* day or current day
    void requeue_manual(const ecf::Calendar& c);           // can match today if today is match day
    void requeue_repeat_increment(const ecf::Calendar& c); // match *FUTURE* day

    // Called after a node has completed, if calendar day corresponds to *THIS* day. *Expire* it
    // This should be called just before: checkForRequeue.
    // This day attribute should be treated as being deleted. returns false from
    //  - isFree()          stops re-queue on expired day
    //  - calendarChanged() stops clearing of free.
    // Expired flag is RESET only by:
    //    - void requeue_manual(const ecf::Calendar& c);
    //    - void requeue_repeat_increment(const ecf::Calendar& c);
    void check_for_expiration(const ecf::Calendar&);

    // We must use a real date, using enum is not sufficient as in ecflow4
    //     0,1,2,3,4,5,6
    //     ^           ^
    //    sunday      saturday
    // old/buggy: check_for_requeue() { return (day_ > calendar.day_of_week() );}
    // The old check for re-queue would determine if this day is in the future, with reference to calendar day.
    // Hence day_(Saturday) is would always re-queue, day_(Sunday) is would never re-queue
    // When multiple days were involved, it would get even buggier.
    // By using a real date, we fix the issue, the real date is updated during manual re-queue, or automatically vi
    // repeat increment.
    bool checkForRequeue(const ecf::Calendar&) const;

    void setFree(); // ensures that isFree() always returns true
    bool isSetFree() const { return free_; }
    void calendarChanged(const ecf::Calendar& c, bool clear_at_midnight = true); // can set attribute free
    bool isFree(const ecf::Calendar&) const;
    bool validForHybrid(const ecf::Calendar&) const;
    bool why(const ecf::Calendar&, std::string& theReasonWhy) const;

    // The state_change_no is never reset. Must be incremented if it can affect equality
    unsigned int state_change_no() const { return state_change_no_; }

    std::string name() const; // for display/gui only
    std::string toString() const;
    std::string dump() const;
    std::string as_simple_string() const;

    // return the days, if input is not valid will throw a runtime_error
    static DayAttr create(const std::string& dayStr);
    static DayAttr create(const std::vector<std::string>& lineTokens, bool read_state);
    static DayAttr::Day_t getDay(const std::string&);

    static std::vector<std::string> allDays();

    // access
    DayAttr::Day_t day() const { return day_; }
    const boost::gregorian::date& date() const { return date_; }

    boost::gregorian::date next_matching_date(const ecf::Calendar& c) const;

    void set_expired(); // ********* TREAT this Day Attribute as deleted **********
    bool expired() const { return expired_; }

    void read_state(const std::vector<std::string>& lineTokens);

private:
    void clearFree(); // resets the free flag
    void clear_expired();
    bool is_free(const ecf::Calendar&) const; // ignores free_
    boost::gregorian::date matching_date(const ecf::Calendar& c) const;
    void write(std::string&) const;

private:
    DayAttr::Day_t day_{DayAttr::SUNDAY};
    unsigned int state_change_no_{0}; // *not* persisted, only used on server side
    bool free_{false};                // persisted for use by why() on client side
    bool expired_{false};             // added for ecflow 5.4.0

    boost::gregorian::date date_; // corresponding to day_

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar);
};

#endif /* ecflow_attribute_DayAttr_HPP */
