/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "DayAttr.hpp"

#include <cassert>
#include <sstream>
#include <stdexcept>

#include "Calendar.hpp"
#include "Ecf.hpp"
#include "Extract.hpp"
#include "Indentor.hpp"
#include "PrintStyle.hpp"
#include "Serialization.hpp"
#include "cereal_boost_time.hpp"

using namespace std;
using namespace ecf;
using namespace boost::gregorian;

// #define DEBUG_DAYS 1

//===============================================================================

static const char* theDay(DayAttr::Day_t day) {
    switch (day) {
        case DayAttr::SUNDAY:
            return "sunday";
            break;
        case DayAttr::MONDAY:
            return "monday";
            break;
        case DayAttr::TUESDAY:
            return "tuesday";
            break;
        case DayAttr::WEDNESDAY:
            return "wednesday";
            break;
        case DayAttr::THURSDAY:
            return "thursday";
            break;
        case DayAttr::FRIDAY:
            return "friday";
            break;
        case DayAttr::SATURDAY:
            return "saturday";
            break;
        default:
            assert(false);
            break;
    }
    return nullptr;
}

//===============================================================================

void DayAttr::calendarChanged(const ecf::Calendar& c, bool clear_at_midnight) {
    // See ECFLOW-337
    //	repeat ....
    //    family start
    //      family 0
    //        time 10:00
    //        day monday        # if there was no c.dayChanged(), then after re-queue, & before midnight Monday is still
    //        free. task dummy        # hence we will end up also running on Tuesday at 10:00
    //          complete 1==1
    //          trigger 0==1
    //
    //  ECFLOW-1550            # If children of a family with day/date are still active/submitted/queued, then don't
    //  clear at midnight. repeat ....            # This is only applicable for NodeContainers, for task with day/date
    //  always CLEAR at midnight
    //	family f1
    //	   day monday
    //	   time 23:00
    //	   task t1             # Task t1 took longer than 1 hour
    //	   task t2             # allow task t2 to continue to the next day, i.e clear_at_midnight = False
    //	      trigger t1 == complete

#ifdef DEBUG_DAYS
    cout << " DayAttr::calendarChanged " << dump() << " clear_at_midnight " << clear_at_midnight
         << " calendar:" << c.suite_time_str() << "\n";
#endif
    if (expired_) {
        // ********* TREAT this Day Attribute as deleted ECFLOW-128 **********
        return;
    }

    if (c.dayChanged()) {
        if (clear_at_midnight) {
            clearFree();
#ifdef DEBUG_DAYS
            cout << " DayAttr::calendarChanged MIDNIGHT " << dump() << " calendar:" << c.suite_time_str() << "\n";
#endif
        }
    }

    if (free_) {
        return;
    }

    if (is_free(c)) {
        setFree();
#ifdef DEBUG_DAYS
        cout << " DayAttr::calendarChanged SET FREE " << dump() << " calendar:" << c.suite_time_str() << "\n";
#endif
    }
}

void DayAttr::reset() {
    expired_         = false;
    free_            = false;
    date_            = boost::gregorian::date();
    state_change_no_ = Ecf::incr_state_change_no();

#ifdef DEBUG_DAYS
    cout << " DayAttr::reset :" << dump() << "\n";
#endif
}

void DayAttr::handle_migration(const ecf::Calendar& c) {
    // <TODO> temp once in Bologna, and when ecflow4 no longer used.
    if (date_.is_special()) {
        if (!c.is_special()) {
            date_ = matching_date(c);
        }
    }
}

void DayAttr::reset(const ecf::Calendar& c) {
    reset();
    date_ = matching_date(c);

#ifdef DEBUG_DAYS
    cout << " DayAttr::reset(calendar) :" << dump() << " calendar:" << c.suite_time_str() << "\n";
#endif
}

void DayAttr::requeue_time() {
#ifdef DEBUG_DAYS
    cout << " DayAttr::requeue " << dump() << "\n";
#endif

    if (expired_) {
        // ********* TREAT this Day Attribute as deleted **********
#ifdef DEBUG_DAYS
        cout << " DayAttr::requeue_time " << dump() << " EXPIRED(do nothing) returning\n";
#endif
        return;
    }

    free_            = false;
    state_change_no_ = Ecf::incr_state_change_no();
}

void DayAttr::requeue_manual(const ecf::Calendar& c) {
    reset();
    date_ = matching_date(c);

#ifdef DEBUG_DAYS
    cout << "  DayAttr::requeue_manual(calendar) " << dump() << " calendar:" << c.suite_time_str() << "\n";
#endif
}

void DayAttr::requeue_repeat_increment(const ecf::Calendar& c) {
    reset();
    date_ = next_matching_date(c);

#ifdef DEBUG_DAYS
    cout << "  DayAttr::requeue_repeat_increment(calendar) " << dump() << " calendar:" << c.suite_time_str() << "\n";
#endif
}

bool DayAttr::isFree(const ecf::Calendar& c) const {
    if (expired_) {
        // ********* TREAT this Day Attribute as deleted **********
#ifdef DEBUG_DAYS
        cout << " DayAttr::isFree " << dump() << " calendar:" << c.suite_time_str() << " HOLDING due to EXPIRED flag\n";
#endif
        return false;
    }

    // The FreeDepCmd can be used to free the dates,
    if (free_) {
#ifdef DEBUG_DAYS
        cout << " DayAttr::isFree " << dump() << " calendar:" << c.suite_time_str() << " FREE free_ = TRUE\n";
#endif
        return true;
    }

    bool res = is_free(c);

#ifdef DEBUG_DAYS
    if (res)
        cout << " DayAttr::isFree " << dump() << " calendar:" << c.suite_time_str() << " date is FREE\n";
    else
        cout << " DayAttr::isFree " << dump() << " calendar:" << c.suite_time_str() << " date is HOLDING\n";
#endif
    return res;
}

bool DayAttr::is_free(const ecf::Calendar& c) const {
    return (c.date() == date_);
}

void DayAttr::setFree() {
    free_            = true;
    state_change_no_ = Ecf::incr_state_change_no();

#ifdef DEBUG_DAYS
    cout << " DayAttr::setFree() " << dump() << "\n";
#endif
}

void DayAttr::clearFree() {
    free_            = false;
    state_change_no_ = Ecf::incr_state_change_no();

#ifdef DEBUG_DAYS
    cout << " DayAttr::clearFree() " << dump() << "\n";
#endif
}

void DayAttr::set_expired() {
    expired_         = true;
    state_change_no_ = Ecf::incr_state_change_no();

#ifdef DEBUG_DAYS
    cout << " DayAttr::set_expired() " << dump() << "\n";
#endif
}

void DayAttr::clear_expired() {
    expired_         = false;
    state_change_no_ = Ecf::incr_state_change_no();

#ifdef DEBUG_DAYS
    cout << " DayAttr::clear_expired() " << dump() << "\n";
#endif
}

void DayAttr::check_for_expiration(const ecf::Calendar& c) {
#ifdef DEBUG_DAYS
    cout << "DayAttr::check_for_expiration " << dump() << " calendar:" << c.suite_time_str() << "\n";
#endif
    // This function is called when a Node has COMPLETED. Avoid running again on same day date <= calendar, expire the
    // date Note: time attribute, they get handled before. i.e allowing multiple re-queues on the same date This
    // function *MUST* be called before checkForRequeue.

    if (date_.is_special()) {
        // migration 4->5, or 5->5 from checkpoint
        date_ = matching_date(c);
    }

    if (day_ == c.day_of_week()) {
        set_expired();
        return;
    }

    if (date_ <= c.date()) {
        set_expired();
    }
}

bool DayAttr::checkForRequeue(const ecf::Calendar& c) const {
    if (expired_) {
#ifdef DEBUG_DAYS
        cout << " DayAttr::check_for_requeue ALREADY EXPIRED " << dump() << " calendar:" << c.suite_time_str()
             << "  HOLDING <<<<<<<<<<<<\n";
#endif
        return false;
    }

    // if calendar is hybrid, we can't requeue
    if (c.hybrid()) {
        return false;
    }

    // checkForRequeue is called when we are deciding whether to re-queue the node
    // Hence we *MUST* have completed. Also crons,time,today have all returned false.
    // *IF* this date is in the future, they we should re-queue

    assert(!date_.is_special());

    bool future_date = (date_ > c.date());

#ifdef DEBUG_DAYS
    cout << " DayAttr::check_for_requeue " << dump() << " calendar:" << c.suite_time_str() << " returning "
         << future_day << " ************\n";
#endif
    return future_date;
}

bool DayAttr::validForHybrid(const ecf::Calendar& calendar) const {
    return isFree(calendar);
}

bool DayAttr::why(const ecf::Calendar& c, std::string& theReasonWhy) const {
    if (isFree(c))
        return false;

    theReasonWhy += " is day dependent ( next run on ";
    theReasonWhy += theDay(day_);
    theReasonWhy += " ";
    if (date_.is_special())
        theReasonWhy += to_simple_string(next_matching_date(c));
    else
        theReasonWhy += to_simple_string(date_);
    theReasonWhy += " the current day is ";
    theReasonWhy += theDay(static_cast<DayAttr::Day_t>(c.day_of_week()));
    theReasonWhy += " )";
    return true;
}

void DayAttr::print(std::string& os) const {
    Indentor in;
    Indentor::indent(os);
    write(os);
    if (!PrintStyle::defsStyle()) {
        bool added_hash = false;
        if (free_) {
            os += " # free";
            added_hash = true;
        }
        if (expired_) {
            if (added_hash)
                os += " expired";
            else
                os += " # expired";
            added_hash = true;
        }

        if (added_hash) {
            os += " date:";
            os += to_simple_string(date_);
        }
        else {
            os += " # date:";
            os += to_simple_string(date_);
        }
    }
    os += "\n";
}

std::string DayAttr::name() const {
    // for display/GUI only
    std::string os;
    write(os);
    bool added_hash = false;

    if (expired_) {
        os += " # expired";
        added_hash = true;
    }
    else {
        if (free_) {
            os += " # free";
            added_hash = true;
        }
    }

    if (added_hash) {
        os += " ";
        os += to_simple_string(date_);
    }
    else {
        os += " # ";
        os += to_simple_string(date_);
    }

    return os;
}

std::string DayAttr::toString() const {
    std::string ret;
    write(ret);
    return ret;
}

void DayAttr::write(std::string& ret) const {
    ret += "day ";
    ret += theDay(day_);
}

std::string DayAttr::dump() const {
    std::stringstream ss;
    ss << toString();
    if (free_)
        ss << " (free)";
    if (expired_)
        ss << " (expired)";
    ss << " " << to_simple_string(date_);
    return ss.str();
}

bool DayAttr::operator==(const DayAttr& rhs) const {
    if (free_ != rhs.free_) {
#ifdef DEBUG
        if (Ecf::debug_equality()) {
            std::cout << "DayAttr::operator== free_ != rhs.free_   (free_:" << free_ << " rhs.free_:" << rhs.free_
                      << ") " << dump() << "\n";
        }
#endif
        return false;
    }
    if (expired_ != rhs.expired_) {
#ifdef DEBUG
        if (Ecf::debug_equality()) {
            std::cout << "DayAttr::operator== expired_ != rhs.expired_   (expired_:" << expired_
                      << " rhs.expired_:" << rhs.expired_ << ") " << dump() << "\n";
        }
#endif
        return false;
    }
    if (date_ != rhs.date_) {
#ifdef DEBUG
        if (Ecf::debug_equality()) {
            std::cout << "DayAttr::operator== date_ != rhs.date_   (date_:" << date_ << " rhs.date_:" << rhs.date_
                      << ") " << dump() << "\n";
        }
#endif
        return false;
    }

    return structureEquals(rhs);
}

bool DayAttr::structureEquals(const DayAttr& rhs) const {
    return (day_ == rhs.day_);
}

DayAttr DayAttr::create(const std::string& dayStr) {
    return DayAttr(getDay(dayStr));
}

DayAttr DayAttr::create(const std::vector<std::string>& lineTokens, bool read_state) {
    if (lineTokens.size() < 2) {
        throw std::runtime_error("DayAttr::create date tokens to short :");
    }

    //   for(size_t i =0; i < lineTokens.size() ; i++) {
    //      cout << "lineTokens[" << i << "] = '" << lineTokens[i] << "'\n";
    //   }

    // day monday  # free expired date:****
    // day monday  # expired
    DayAttr day = DayAttr::create(lineTokens[1]);

    // state
    if (read_state) {
        day.read_state(lineTokens);
    }
    return day;
}

void DayAttr::read_state(const std::vector<std::string>& lineTokens) {
    std::string date;

    for (size_t i = 3; i < lineTokens.size(); i++) {
        if (lineTokens[i] == "free")
            free_ = true;
        else if (lineTokens[i] == "expired")
            expired_ = true;
        else if (lineTokens[i].find("date:") != std::string::npos) {
            if (!Extract::split_get_second(lineTokens[i], date))
                throw std::runtime_error("DayAttr::read_state failed: (date:)");
            // when a date_ is special date = not-a-date-time\n"
            if (date.find("not") == std::string::npos) {
                date_ = from_simple_string(date);
            }
        }
    }
}

DayAttr::Day_t DayAttr::getDay(const std::string& day) {
    if (day == "monday")
        return DayAttr::MONDAY;
    if (day == "tuesday")
        return DayAttr::TUESDAY;
    if (day == "wednesday")
        return DayAttr::WEDNESDAY;
    if (day == "thursday")
        return DayAttr::THURSDAY;
    if (day == "friday")
        return DayAttr::FRIDAY;
    if (day == "saturday")
        return DayAttr::SATURDAY;
    if (day == "sunday")
        return DayAttr::SUNDAY;

    std::stringstream ss;
    ss << "Invalid day(" << day
       << ") specification expected one of [monday,tuesday,wednesday,thursday,friday,saturday,sunday]: ";
    throw std::runtime_error(ss.str());

    return DayAttr::SUNDAY;
}

std::vector<std::string> DayAttr::allDays() {
    std::vector<std::string> vec;
    vec.reserve(7);
    vec.emplace_back("monday");
    vec.emplace_back("tuesday");
    vec.emplace_back("wednesday");
    vec.emplace_back("thursday");
    vec.emplace_back("friday");
    vec.emplace_back("saturday");
    vec.emplace_back("sunday");
    return vec;
}

boost::gregorian::date DayAttr::matching_date(const ecf::Calendar& c) const {
    boost::gregorian::date_duration one_day(1);
    boost::gregorian::date matching_date = c.date(); // todays date

    for (int i = 0; i < 7; i++) {
        if (matching_date.day_of_week().as_number() == day_) {
            return matching_date;
        }
        matching_date += one_day;
    }
    assert(false); // no matching day ?s
    return c.date();
}

boost::gregorian::date DayAttr::next_matching_date(const ecf::Calendar& c) const {
    boost::gregorian::date_duration one_day(1);
    boost::gregorian::date the_next_matching_date = c.date(); // todays date

    for (int i = 0; i < 7; i++) {
        the_next_matching_date += one_day;
        if (the_next_matching_date.day_of_week().as_number() == day_) {
            return the_next_matching_date;
        }
    }
    assert(false);
    return c.date();
}

template <class Archive>
void DayAttr::serialize(Archive& ar) {
    ar(CEREAL_NVP(day_));
    CEREAL_OPTIONAL_NVP(ar, free_, [this]() { return free_; }); // conditionally save
    CEREAL_OPTIONAL_NVP(ar, expired_, [this]() {
        return expired_;
    }); // conditionally save, new to ecflow 5.4.0, should be ignored by old clients. see tests
    CEREAL_OPTIONAL_NVP(ar, date_, [this]() {
        return !date_.is_special();
    }); // conditionally save, new to ecflow 5.5.0, should be ignored by old clients. see tests
}
CEREAL_TEMPLATE_SPECIALIZE(DayAttr);
