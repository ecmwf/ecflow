/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/core/TimeSeries.hpp"

#include <stdexcept>

#include "ecflow/core/Calendar.hpp"
#include "ecflow/core/Chrono.hpp"
#include "ecflow/core/Extract.hpp"
#include "ecflow/core/Log.hpp"
#include "ecflow/core/Serialization.hpp"
#include "ecflow/core/Str.hpp"
#include "ecflow/core/cereal_boost_time.hpp"
#ifdef DEBUG
    #include "ecflow/core/Ecf.hpp"
#endif

using namespace std;
using namespace ecf;

static void testTimeSlot(const ecf::TimeSlot& ts) {
    if (ts.hour() < 0 || ts.hour() > 23) {
        std::stringstream ss;
        ss << "TimeSeries:  time hour(" << ts.hour() << ") must be in range 0-23";
        throw std::out_of_range(ss.str());
    }
    if (ts.minute() < 0 || ts.minute() > 59) {
        std::stringstream ss;
        ss << "TimeSeries:  time minute(" << ts.minute() << ") must be in range 0-59";
        throw std::out_of_range(ss.str());
    }
}

namespace ecf {

TimeSeries::TimeSeries() = default;

TimeSeries::TimeSeries(int hour, int minute, bool relative)
    : start_(hour, minute),
      nextTimeSlot_(hour, minute),
      lastTimeSlot_(start_.duration()),
      relativeToSuiteStart_(relative) {
    testTimeSlot(start_);
}

TimeSeries::TimeSeries(const TimeSlot& t, bool relative)
    : start_(t),
      nextTimeSlot_(t),
      lastTimeSlot_(t.duration()),
      relativeToSuiteStart_(relative) {
    testTimeSlot(start_);
}

TimeSeries::TimeSeries(const TimeSlot& start, const TimeSlot& finish, const TimeSlot& incr, bool relative)
    : start_(start),
      finish_(finish),
      incr_(incr),
      nextTimeSlot_(start),
      relativeToSuiteStart_(relative) {
    testTimeSlot(start);
    testTimeSlot(finish);
    testTimeSlot(incr);

    if (!finish_.isNULL()) {
        if (incr_.isNULL()) {
            std::stringstream ss;
            ss << "TimeSeries::TimeSeries: Invalid time series: Finish specified without an increment";
            throw std::out_of_range(ss.str());
        }
    }

    if (start.duration() > finish.duration()) {
        std::stringstream ss;
        ss << "TimeSeries::TimeSeries: Invalid time series: Start time(" << start.toString()
           << ") is greater than end time(" << finish.toString() << ")";
        throw std::out_of_range(ss.str());
    }
    if (incr.hour() == 0 && incr.minute() == 0) {
        throw std::out_of_range(
            "TimeSeries::TimeSeries Invalid time series:  Increment must be greater than 0 minutes.");
    }
    auto diff = finish.duration() - start.duration();
    if (incr.duration() > diff) {
        std::stringstream ss;
        ss << "TimeSeries::TimeSeries: Invalid time series: Increment(" << incr.toString()
           << ") is greater than duration " << to_simple_string(diff) << " between start(" << start.toString()
           << ") and finish(" << finish.toString() << ")";
        throw std::out_of_range(ss.str());
    }

    compute_last_time_slot();

    // #ifdef DEBUG_TIME_SERIES
    //	cout << "TimeSeries::TimeSeries "  << dump() << "\n";
    // #endif
}

bool TimeSeries::operator<(const TimeSeries& rhs) const {
    return start_ < rhs.start_;
}

void TimeSeries::compute_last_time_slot() {
    if (!finish_.isNULL()) {
        lastTimeSlot_ = start_.duration();
        while (lastTimeSlot_ <= finish_.duration()) {
            lastTimeSlot_ += incr_.duration();
        }
        lastTimeSlot_ -= incr_.duration();
    }
}

bool TimeSeries::calendarChanged(const ecf::Calendar& c) {
    if (relativeToSuiteStart_) {
        relativeDuration_ += c.calendarIncrement();
        return true;
    }
    else if (c.dayChanged()) {

        // Clear expired flag and next slot. Needed since requeuing will expire flag when pastime slot
        // Hence we need something to reset. Otherwise, for next day isFree will always return false;
        isValid_      = true;
        nextTimeSlot_ = start_;
        return true;
    }
    return false;
}

bool TimeSeries::resetRelativeDuration() {
    if (relativeToSuiteStart_) {
        relativeDuration_ = boost::posix_time::time_duration(0, 0, 0, 0);

#ifdef DEBUG_TIME_SERIES
        log(Log::DBG, "TimeSeries::resetRelativeDuration " + dump());
#endif

        return true;
    }
#ifdef DEBUG_TIME_SERIES
    log(Log::DBG, "TimeSeries::resetRelativeDuration " + dump());
#endif
    return false;
}

void TimeSeries::reset_only() {
    isValid_      = true;
    nextTimeSlot_ = start_;
    (void)resetRelativeDuration();
}

void TimeSeries::reset(const ecf::Calendar& c) {
    reset_only();

    // Note: **difference between reset and re-queue,
    //  Hence if at begin(), time slot same as current time, allow job to run.
    //  reset  : while( current_time >  nextTimeSlot_.duration()) {  // need for why command
    //              if (current_time >  start_.duration() ) {
    //  requeue: while( current_time >= nextTimeSlot_.duration()) {
    //              if (current_time >= start_.duration() ) {

    // Update nextTimeSlot_ so that why command works out of the box, when nodes have been begun.
    // *if* the current time is *AT* the start do *not* increment nextTimeSlot_, otherwise we will miss first time slot
    auto current_time = duration(c);
    if (hasIncrement()) {

        // only used when we have a series, and *NOT* relative. A relative time series does not care about midnight.
        if (!relativeToSuiteStart_) {
            suiteTimeAtRequeue_ = TimeSlot(c.suiteTime().time_of_day());
        }

        while (current_time > nextTimeSlot_.duration()) {
            auto value = nextTimeSlot_.duration();
            value += incr_.duration();
            nextTimeSlot_ = TimeSlot(value.hours(), value.minutes());
        }
        if (nextTimeSlot_ > finish_) {
            isValid_ = false; // time has expired
        }
    }
    else {
        if (current_time > start_.duration()) {
            isValid_ = false; // time has expired
        }
    }

#ifdef DEBUG_TIME_SERIES
    //	LogToCout toCoutAsWell;
    log(Log::DBG, "TimeSeries::reset   " + dump());
#endif
}

void TimeSeries::requeue(const ecf::Calendar& c, bool reset_next_time_slot) {
    // cout << "TimeSeries::requeue " << c.toString()  << "\n";

    // *RESET* to handle case where time slot has been advanced, but at requeue it must be reset
    // This is important otherwise user can never reset and time slot that had been advanced
    // by using miss_next_time_slot()
    if (reset_next_time_slot) {
        isValid_      = true;
        nextTimeSlot_ = start_;
    }

    //   time 13:00 // nextTimeSlot_ is initialised to 13:00, on TimeSeries::requeue() invalidate time series
    //              // to stop multiple job submissions on same time slot
    //                       -------------------- nextTimeSlot_
    //                       |     -------------- isValid = false
    //                       V     V
    //   10:00 11:00 12:00 13:00 14:00 15:00 16:00 17:00 18:00 19:00 20:00
    //     |     |     |     |     |     |     |     |     |     |     |
    //  ------time----------->
    //
    // TimeSeries::requeue(..) is called at the *re-queue* stage. *after*:
    //   a/ task has completed
    //   b/ checkForRequeue() has passed.
    // hence if we get here for a single slot time, where calendar time >= start time
    // then this time series is no longer valid. This will stop multiple job submission
    // for the same time slot
    auto current_time = duration(c);
    if (!hasIncrement()) {
        if (current_time >= start_.duration()) {
            isValid_ = false; // time has expired
#ifdef DEBUG_TIME_SERIES
            LOG(Log::DBG,
                "TimeSeries::requeue HOLDING TIME EXPIRED (duration(c) >= start_.duration() ) "
                    << dump() << " duration=" << to_simple_string(duration(c)));
#endif
        }
        return;
    }

    // Only used when we have a series, and NOT relative. A relative time series does not care about midnight
    if (!relativeToSuiteStart_) {
        suiteTimeAtRequeue_ = TimeSlot(c.suiteTime().time_of_day());
    }

    // the nextTimeSlot_ needs to be set to a multiple of incr
    // However the nextTimeSlot_ cannot just be incremented by incr
    // since we can't assume that a task completes within the given time slots,
    // hence, increments to NEXT TIME SLOT larger than calendar time.
    // time 10::00 20:00 01:00
    //                       --------------------------------------------------------nextTimeSlot_ must greater than
    //                       current time. |                                               --------isValid = false V V
    //   10:00 11:00 12:00 13:00 14:00 15:00 16:00 17:00 18:00 19:00 20:00 21:00
    //     |     |     |     |     |     |     |     |     |     |     |     |
    //  ------time---->
    //
    while (current_time >= nextTimeSlot_.duration()) {
        auto value = nextTimeSlot_.duration();
        value += incr_.duration();
        nextTimeSlot_ = TimeSlot(value.hours(), value.minutes());
    }

    if (nextTimeSlot_ > finish_) {
        isValid_            = false;      // time has expired
        suiteTimeAtRequeue_ = TimeSlot(); // expire for new requeue
#ifdef DEBUG_TIME_SERIES
        log(Log::DBG, "TimeSeries::requeue  HOLDING TIME EXPIRED (nextTimeSlot_ > finish_) " + dump());
#endif
    }
}

TimeSlot TimeSeries::compute_next_time_slot(const ecf::Calendar& c) const {
    // This functionality needs to mirror TimeSeries::requeue
    auto current_time = duration(c);
    if (!hasIncrement()) {
        if (current_time >= start_.duration()) {
            return {}; // time has expired
        }
        return start_;
    }

    TimeSlot nextTimeSlot = start_;
    while (current_time >= nextTimeSlot.duration()) {
        auto value = nextTimeSlot.duration();
        value += incr_.duration();
        nextTimeSlot = TimeSlot(value.hours(), value.minutes());
    }

    if (nextTimeSlot > finish_) {
        return {}; // time has expired
    }
    return nextTimeSlot;
}

bool TimeSeries::requeueable(const ecf::Calendar& c) const {
    auto calendar_time = duration(c);
    if (calendar_time < start().duration()) {
        return true;
    }
    if (hasIncrement()) {
        if (calendar_time < finish().duration()) {
            return true;
        }
    }
    return false;
}

bool TimeSeries::isFree(const ecf::Calendar& calendar) const {
#ifdef DEBUG_TIME_SERIES_IS_FREE
    LogToCout toCoutAsWell;
    Indentor indent;
#endif

    if (!isValid_) {
        // time has expired, hence time is not free
#ifdef DEBUG_TIME_SERIES_IS_FREE
        log(Log::DBG, "TimeSeries::isFree (!isValid_) HOLDING " + dump());
#endif
        return false;
    }

    // Matched calendar duration with the current value of the time series
    // or match with one of time slots.

    // Note:: the definition file has time series format of hh:mm this
    //        means we have a minute resolution. The clock/calendar
    //        duration has seconds based resolution. hence we must
    //        compensate for this.
    //
    //   time 10:00 20:00 01:00
    //   start
    //     |                 ----- next time slot                      --- finish
    //     |                 |                                         |
    //     V                 V                                         V
    //   10:00 11:00 12:00 13:00 14:00 15:00 16:00 17:00 18:00 19:00 20:00
    //     |     |     |     |     |     |     |     |     |     |     |
    //  ------time---->
    //
    bool ret = match_duration_with_time_series(duration(calendar));
    //	if (ret) {
    //	   std::cout << "TimeSeries::isFree " << dump() << " is free at calendar: " << calendar.toString() << "\n";
    //	}
    return ret;
}

bool TimeSeries::match_duration_with_time_series(const boost::posix_time::time_duration& relative_or_real_td) const {
#ifdef DEBUG_TIME_SERIES_IS_FREE
    Indentor ident;
#endif

    if (!hasIncrement()) {
        // We ignore seconds, hence +00:02  will match 2.58 (two minutes 58 seconds) relative duration
        auto start_td = start_.duration();
        if (relative_or_real_td.hours() == start_td.hours() && relative_or_real_td.minutes() == start_td.minutes()) {
#ifdef DEBUG_TIME_SERIES_IS_FREE
            LOG(Log::DBG,
                "TimeSeries::match_duration_with_time_series " << dump() << " FREE at "
                                                               << to_simple_string(relative_or_real_td));
#endif
            return true;
        }

#ifdef DEBUG_TIME_SERIES_IS_FREE
        LOG(Log::DBG,
            "TimeSeries::match_duration_with_time_series " << dump() << " HOLDING at "
                                                           << to_simple_string(relative_or_real_td));
#endif
        return false;
    }

    auto endDuration     = finish_.duration();
    auto incrDuration    = incr_.duration();
    auto nextTimeSlot_td = nextTimeSlot_.duration();
    long hours           = relative_or_real_td.hours();
    long minutes         = relative_or_real_td.minutes();
    while (nextTimeSlot_td <= endDuration) {

        if (hours == nextTimeSlot_td.hours() && minutes == nextTimeSlot_td.minutes()) {
#ifdef DEBUG_TIME_SERIES_IS_FREE
            LOG(Log::DBG,
                "TimeSeries::match_duration_with_time_series (nextTimeSlot_td == duration)  "
                    << dump() << " FREE at " << to_simple_string(relative_or_real_td));
#endif
            return true;
        }
        nextTimeSlot_td += incrDuration;
    }

#ifdef DEBUG_TIME_SERIES
    LOG(Log::DBG,
        "TimeSeries::match_duration_with_time_series HOLDING (nextTimeSlot_td > endDuration)  "
            << dump() << " HOLDING at " << to_simple_string(relative_or_real_td));
#endif
    return false;
}

void TimeSeries::miss_next_time_slot() {
    if (!hasIncrement()) {
        // single slot, does not have a next time slot, hence expire time
        isValid_ = false;
    }
    else {
        auto value = nextTimeSlot_.duration();
        value += incr_.duration();
        nextTimeSlot_ = TimeSlot(value.hours(), value.minutes());
        if (nextTimeSlot_ > finish_) {
            // time has expired,
            isValid_ = false;
        }
    }
}

bool TimeSeries::checkForRequeue(const ecf::Calendar& calendar,
                                 const TimeSlot& the_min,
                                 const TimeSlot& the_max,
                                 bool cmd_context) const {
    // ************************************************************************
    // THIS IS CALLED IN THE CONTEXT WHERE NODE HAS COMPLETED. Hence ****asynchronous****
    // RETURNING TRUE FROM HERE WILL FORCE NODE TO QUEUED STATE
    // HENCE THIS FUNCTION MUST RETURN FALSE, WHEN END OF TIME SLOT HAS BEEN REACHED/expired
    // The resolution is in minutes
    // *************************************************************************
    // cout << "TimeSeries::checkForRequeue " << calendar.suite_time_str() << " min: " << the_min << " max: " << the_max
    // << " cmd_context(" << cmd_context << ") +++++++++++++++\n";
#ifdef DEBUG_TIME_SERIES
    log(Log::DBG, "TimeSeries::checkForRequeue: " + dump());
#endif

    if (!isValid_) {
        // time has expired, hence can no longer re-queues, i.e. no future time dependency
#ifdef DEBUG_TIME_SERIES
        // cout << " TimeSeries::checkForRequeue " << calendar.suite_time_str() << "  HOLDING\n";
        log(Log::DBG, "TimeSeries::checkForRequeue HOLDING !isValid_  " + dump());
#endif
        return false;
    }

    if (hasIncrement()) {
        // Note if we are equal to the finish and were called as part of completeCmd
        // then completeCmd will initiate a job submission immediately
        //  start  00:01
        //  finish 00:04   Node will be queued 4 times
        //  incr   00:01

        // If the current value is greater that finish, then returning true would increment
        // value past the end, and force node state to be stuck in state queue.
        if (nextTimeSlot_ > finish_) {
#ifdef DEBUG_TIME_SERIES
            // cout << " TimeSeries::checkForRequeue ( nextTimeSlot_ > finish_ )    HOLDING\n";
            log(Log::DBG, "TimeSeries::checkForRequeue HOLDING  ( nextTimeSlot_ > finish_ )  " + dump());
#endif
            return false;
        }

        // ECFLOW-130 jobs that start before midnight and finish after midnight should not re-queue
        // This should ONLY apply to non-relative time series, since a relative time series does care about midnight
        if (!relativeToSuiteStart_ && !suiteTimeAtRequeue_.isNULL()) {
            TimeSlot suiteTimeNow(calendar.suiteTime().time_of_day());
            // cout << "TimeSeries::checkForRequeue suiteTimeNow = " << suiteTimeNow << "
            // =====================================================\n";
            //  we use >= specifically for unit test, to pass.
            if (suiteTimeNow >= suiteTimeAtRequeue_) {
                // normal flow, i.e. same day
                suiteTimeAtRequeue_ = TimeSlot(); // make NULL, allow requeuing to reset.
            }
            else {
                // The day changed between (requeue/reset):->queued->submitted->active->complete->(checkForRequeue)
#ifdef DEBUG_TIME_SERIES
                // cout << " TimeSeries::checkForRequeue day changed
                // ===================================================== HOLDING\n";
                log(Log::DBG, "TimeSeries::checkForRequeue HOLDING suiteTimeNow < suiteTimeAtReque_  " + dump());
#endif
                return false;
            }
        }

        auto calendar_duration = duration(calendar);
        if (cmd_context) {
            // In the *COMMAND* context, allow re-queue if we are *BEFORE* the last time slot
            if (calendar_duration < lastTimeSlot_) {
                return true;
            }
        }
        else {
            if (calendar_duration >= start_.duration() && calendar_duration < lastTimeSlot_) {
                return true;
            }
        }

#ifdef DEBUG_TIME_SERIES
        log(Log::DBG,
            "TimeSeries::checkForRequeue  HOLDING calendar_duration < start_ || calendar_duration >= lastTimeSlot_)  " +
                dump());
#endif
        return false;
    }

    // *** When we have a single time slots we cannot make a decision, whether
    // *** we should re-queue based on this attribute *alone*. (i.e. when we have > 1 time/today attributes)
    // *** Hence we pass min and max time slots, over all the time bases attributes of the same kind.
    // *** In our case we only do this for Time and Today attributes.
    // *** The the_min/the_max have been computed for all attribute (i.e. single time slots and ranges)

    // We have a single time slot, *OR* multiple with same time slot
    if (the_min == the_max) {
        // cout << " TimeSeries::checkForRequeue (the_min == the_max) HOLDING \n";
#ifdef DEBUG_TIME_SERIES
        log(Log::DBG, "TimeSeries::checkForRequeue  HOLDING  the_min == the_max  " + dump());
#endif
        return false;
    }

    // The the_min/the_max takes into account *all* start/finish Time and Today attributes
    auto calendar_duration = duration(calendar);
    if (cmd_context) {
        // In the *COMMAND* context, allow re-queue if we are *BEFORE* the max time slot
        if (calendar_duration < the_max.duration()) {
            // cout << " TimeSeries::checkForRequeue *COMMAND* context calendar_duration < the_max.duration()) --> allow
            // requeue\n";
            return true;
        }
    }
    else {
        if (calendar_duration >= the_min.duration() && calendar_duration < the_max.duration()) {
            // cout << " TimeSeries::checkForRequeue (calendar_duration > the_min.duration() && calendar_duration <
            // the_max.duration()) --> allow requeue\n";
            return true;
        }
    }

    // cout << " TimeSeries::checkForRequeue calendar duration " << to_simple_string(calendar_duration) << " not in
    // range " <<  to_simple_string(the_min.duration())  << "-" <<  to_simple_string(the_max.duration()) << "  HOLDING
    // \n";
#ifdef DEBUG_TIME_SERIES
    log(Log::DBG, "TimeSeries::checkForRequeue HOLDING multiple slots not in range " + dump());
#endif
    return false;
}

void TimeSeries::min_max_time_slots(TimeSlot& the_min, TimeSlot& the_max) const {
    if (the_min.isNULL() || start_ < the_min) {
        the_min = start_;
    }
    if (the_max.isNULL() || start_ > the_max) {
        the_max = start_;
    }
    if (hasIncrement()) {
        if (finish_ < the_min) {
            the_min = finish_;
        }
        if (finish_ > the_max) {
            the_max = finish_;
        }
    }
}

void TimeSeries::why(const ecf::Calendar& c, std::string& theReasonWhy) const {
    std::stringstream ss;
    ss << " ( next run time is ";
    if (relativeToSuiteStart_) {
        ss << "+";
    }
    ss << nextTimeSlot_.toString();

    TimeSlot currentTime = TimeSlot(duration(c));
    ss << ", current suite time is ";
    if (relativeToSuiteStart_) {
        ss << "+";
    }
    ss << currentTime.toString() << " )";
    theReasonWhy += ss.str();
}

boost::posix_time::time_duration TimeSeries::duration(const ecf::Calendar& c) const {
    // return with a minute resolution
    if (relativeToSuiteStart_) {

        // relative to suite start only
        return {relativeDuration_.hours(), relativeDuration_.minutes(), 0, 0};
    }

    LOG_ASSERT(!c.suiteTime().is_special(), "init has not been called on calendar. TimeSeries::duration");
    auto time_of_day = c.suiteTime().time_of_day();
    return {time_of_day.hours(), time_of_day.minutes(), 0, 0};
}

void TimeSeries::free_slots(std::vector<boost::posix_time::time_duration>& vec) const {
    if (hasIncrement()) {

        auto i            = start_.duration();
        auto endDuration  = finish_.duration();
        auto incrDuration = incr_.duration();
        while (i < endDuration) {
            vec.push_back(i);
            i += incrDuration;
        }
        vec.push_back(finish_.duration());
        return;
    }

    vec.push_back(start_.duration());
}

bool TimeSeries::structureEquals(const TimeSeries& rhs) const {
    if (relativeToSuiteStart_ != rhs.relativeToSuiteStart_) {
        return false;
    }
    if (start_ != rhs.start_) {
        return false;
    }
    if (finish_ != rhs.finish_) {
        return false;
    }
    if (incr_ != rhs.incr_) {
        return false;
    }
    return true;
}

bool TimeSeries::operator==(const TimeSeries& rhs) const {
    // additional state
    if (isValid_ != rhs.isValid_) {
#ifdef DEBUG
        if (Ecf::debug_equality()) {
            std::cout << "TimeSeries::operator==  ( isValid_ != rhs.isValid_) " << toString() << "\n";
        }
#endif
        return false;
    }
    if (nextTimeSlot_ != rhs.nextTimeSlot_) {
#ifdef DEBUG
        if (Ecf::debug_equality()) {
            std::cout << "TimeSeries::operator==  ( nextTimeSlot_(" << nextTimeSlot_.toString()
                      << ") != rhs.nextTimeSlot_(" << rhs.nextTimeSlot_.toString() << ")) " << toString() << "\n";
        }
#endif
        return false;
    }
    if (relativeDuration_ != rhs.relativeDuration_) {
#ifdef DEBUG
        if (Ecf::debug_equality()) {
            std::cout << "TimeSeries::operator==  ( relativeDuration_ != rhs.relativeDuration_) " << toString() << "\n";
        }
#endif
        return false;
    }
    return structureEquals(rhs);
}

void TimeSeries::print(std::string& os) const {
    write(os);
    os += "\n";
}

std::string TimeSeries::toString() const {
    std::string ret;
    write(ret);
    return ret;
}

void TimeSeries::write(std::string& ret) const {
    if (relativeToSuiteStart_) {
        ret += "+";
    }
    start_.write(ret);
    if (!finish_.isNULL()) {
        ret += " ";
        finish_.write(ret);
        ret += " ";
        incr_.write(ret);
    }
}

std::string TimeSeries::dump() const {
    std::stringstream ss;
    ss << toString();
    ss << " isValid_(" << isValid_ << ")";
    ss << " nextTimeSlot_(" << nextTimeSlot_.toString() << ")";
    ss << " relativeDuration_(" << to_simple_string(relativeDuration_) << ")";
    ss << " lastTimeSlot_(" << to_simple_string(lastTimeSlot_) << ")";
    ss << " suiteTimeAtRequeue_(" << suiteTimeAtRequeue_.toString() << ")";
    return ss.str();
}

bool TimeSeries::checkInvariants(std::string& errormsg) const {
    if (!finish_.isNULL()) {
        if (incr_.isNULL()) {
            errormsg += "TimeSeries::checkInvariants increment cannot be NULL when we have a time series";
            cout << errormsg << "  " << toString() << "\n";
            return false;
        }
        if (incr_.hour() == 0 && incr_.minute() == 0) {
            errormsg += "TimeSeries::checkInvariants increment must greater than zero";
            cout << errormsg << "  " << toString() << "\n";
            return false;
        }

        if (start_.duration() > finish_.duration()) {
            errormsg += "TimeSeries::checkInvariants Invalid time series start() > finish()";
            cout << errormsg << "  " << toString() << "\n";
            return false;
        }

        if (lastTimeSlot_ <= start_.duration() && lastTimeSlot_ > finish_.duration()) {
            errormsg += "TimeSeries::checkInvariants Invalid last time slot";
            cout << errormsg << "  " << toString() << "\n";
            return false;
        }
    }
    if (relativeDuration_.is_special()) {
        errormsg += "TimeSeries::checkInvariants relativeDuration_ should not be special";
        cout << errormsg << "  " << toString() << "\n";
        return false;
    }

    if (!relativeToSuiteStart_ && relativeDuration_.total_seconds() > 0) {
        errormsg += "TimeSeries::checkInvariants Can only have RelativeDuration if relativeToSuiteStart_ flag is set";
        cout << errormsg << "  " << toString() << "\n";
        return false;
    }

    return true;
}

std::ostream& operator<<(std::ostream& os, const TimeSeries* d) {
    if (d) {
        string s;
        d->print(s);
        os << s;
        return os;
    }
    return os << "TimeSlot == NULL";
}
std::ostream& operator<<(std::ostream& os, const TimeSeries& d) {
    string s;
    d.print(s);
    os << s;
    return os;
}

ecf::TimeSeries TimeSeries::create(const std::string& str) {
    std::vector<std::string> lineTokens;
    Str::split(str, lineTokens);
    size_t index = 0;
    return TimeSeries::create(index, lineTokens);
}

std::string TimeSeries::state_to_string(bool isFree) const {
    // *IMPORTANT* we *CANT* use ';' character, since is used in the parser, when we have
    //             multiple statement on a single line i.e.
    //                 task a; task b;
    //             Hence use of '/' character
    // time 10:30 # free isValid:false nextTimeSlot/10:30 relativeDuration/00:00:00
    std::string ret;
    write_state(ret, isFree);
    return ret;
}

void TimeSeries::write_state(std::string& ret, bool isFree) const {
    // *IMPORTANT* we *CANT* use ';' character, since is used in the parser, when we have
    //             multiple statement on a single line i.e.
    //                 task a; task b;
    //             Hence use of '/' character
    // time 10:30 # free isValid:false nextTimeSlot/10:30 relativeDuration/00:00:00
    bool next_time_slot_changed    = (nextTimeSlot_ != start_);
    bool relative_duration_changed = (!relativeDuration_.is_special() && relativeDuration_.total_seconds() != 0);
    if (isFree || !isValid_ || next_time_slot_changed || relative_duration_changed) {
        ret += " #";
        if (isFree) {
            ret += " free";
        }
        if (!isValid_) {
            ret += " isValid:false";
        }
        if (next_time_slot_changed) {
            ret += " nextTimeSlot/";
            ret += nextTimeSlot_.toString();
        }
        if (relative_duration_changed) {
            ret += " relativeDuration/";
            ret += to_simple_string(relativeDuration_);
        }
    }
}

void TimeSeries::write_state_for_gui(std::string& ret, bool isFree) const {
    bool next_time_slot_changed    = (nextTimeSlot_ != start_);
    bool relative_duration_changed = (!relativeDuration_.is_special() && relativeDuration_.total_seconds() != 0);
    if (isFree || !isValid_ || next_time_slot_changed || relative_duration_changed) {
        ret += " #";
        if (isFree) {
            ret += " free";
        }
        if (!isValid_) {
            ret += " expired";
        }
        if (next_time_slot_changed) {
            ret += " nextTimeSlot=";
            ret += nextTimeSlot_.toString();
        }
        if (relative_duration_changed) {
            ret += " relDuration=";
            ret += to_simple_string(relativeDuration_);
        }
    }
}

void TimeSeries::parse_state(size_t index, const std::vector<std::string>& lineTokens, ecf::TimeSeries& ts) {
    // *IMPORTANT* we *CANT* use ';' character, since is used in the parser, when we have
    //             multiple statement on a single line i.e.
    //                 task a; task b;
    //             Hence use of '/' character
    //
    // Here free is attribute state & not time series state hence ignore
    // time 10:30              # free isValid:false nextTimeSlot/10:30 relativeDuration/00:00:00
    // cron 10:00 20:00 01:00  # free isValid:false nextTimeSlot/10:30 relativeDuration/00:00:00
    bool comment_fnd        = false;
    size_t line_tokens_size = lineTokens.size();
    for (size_t i = index; i < line_tokens_size; i++) {
        if (comment_fnd) {
            if (lineTokens[i] == "isValid:false") {
                ts.isValid_ = false;
                continue;
            }
            if (lineTokens[i].find("nextTimeSlot") != std::string::npos) {
                std::string nextTimeSlot;
                if (Extract::split_get_second(lineTokens[i], nextTimeSlot, '/')) {
                    // Note: we do *not* check for valid time since nextTimeSlot, relative times can be incremented past
                    // 24 hours, i.e. cron +00:00 18:00 06:00 # isValid:false nextTimeSlot/523:00
                    int startHour = -1;
                    int startMin  = -1;
                    getTime(nextTimeSlot, startHour, startMin, false /*check_time*/);
                    ts.nextTimeSlot_ = TimeSlot(startHour, startMin);
                }
                else {
                    throw std::runtime_error("TimeSeries::parse_state: could not extract state.");
                }
            }
            if (lineTokens[i].find("relativeDuration") != std::string::npos) {
                std::string relativeDuration;
                if (Extract::split_get_second(lineTokens[i], relativeDuration, '/')) {
                    ts.relativeDuration_ = boost::posix_time::duration_from_string(relativeDuration);
                }
                else {
                    throw std::runtime_error("TimeSeries::parse_state: could not extract state.");
                }
            }
        }
        if (lineTokens[i] == "#") {
            comment_fnd = true;
        }
    }
    ts.compute_last_time_slot();
}

ecf::TimeSeries TimeSeries::create(size_t& index, const std::vector<std::string>& lineTokens, bool read_state) {
    size_t line_tokens_size = lineTokens.size();
    assert(index < line_tokens_size);
    int startHour = -1;
    int startMin  = -1;

    // cron 10:00 20:00 01:00
    // index is on 10:00, ie index should have value of 1 in this case
    string startStr = lineTokens[index];
    bool relative   = false;
    if (startStr[0] == '+') {
        relative = true;
        startStr.erase(startStr.begin()); // remove leading +
                                          // string must be of form 12:00
    }
    getTime(startStr, startHour, startMin);
    TimeSlot start(startHour, startMin);

    index++; // on 20:00
    if (index < line_tokens_size && lineTokens[index][0] != '#') {

        // if third token is not a comment the time must be of the form
        // cron 10:00 20:00 01:00
        if (index + 1 >= line_tokens_size) {
            throw std::runtime_error("TimeSeries::create: Invalid time series :");
        }

        int finishHour = -1;
        int finishMin  = -1;
        getTime(lineTokens[index], finishHour, finishMin);
        TimeSlot finish(finishHour, finishMin);

        index++;

        int incrHour = -1;
        int incrMin  = -1;
        getTime(lineTokens[index], incrHour, incrMin);
        TimeSlot incr(incrHour, incrMin);

        if (read_state) {
            TimeSeries ts(start, finish, incr, relative);
            parse_state(index, lineTokens, ts);
            return ts;
        }
        return TimeSeries(start, finish, incr, relative);
    }

    if (read_state) {
        TimeSeries ts(start, relative);
        parse_state(index, lineTokens, ts);
        return ts;
    }
    return TimeSeries(start, relative);
}

bool TimeSeries::getTime(const std::string& time, int& hour, int& min, bool check_time) {
    // HH:MM
    // +HH:MM  for other clients
    size_t colonPos = time.find_first_of(':');
    if (colonPos == string::npos) {
        throw std::runtime_error("TimeSeries::getTime: Invalid time :'" + time + "'");
    }

    std::string theHour;
    bool relative = false;
    if (time[0] == '+') {
        relative = true;
        theHour  = time.substr(1, colonPos - 1);
    }
    else {
        theHour = time.substr(0, colonPos);
    }

    std::string theMin = time.substr(colonPos + 1);

    if (check_time && theHour.size() != 2) {
        throw std::runtime_error("TimeSeries::getTime: Invalid hour :" + theHour);
    }
    if (theMin.size() != 2) {
        throw std::runtime_error("TimeSeries::getTime: Invalid minute :" + theMin);
    }

    hour = Extract::value<int>(theHour, "TimeSeries::getTime: hour must be a integer : " + theHour);
    min  = Extract::value<int>(theMin, "TimeSeries::getTime: minute must be integer : " + theMin);

    if (check_time) {
        testTime(hour, min);
    }
    return relative;
}

void TimeSeries::testTime(int hour, int minute) {
    if (hour == -1 || minute == -1) {
        throw std::runtime_error("TimeSeries::testTime: Failed to extract time");
    }
    if (hour < 0 || hour > 23) {
        std::stringstream ss;
        ss << "TimeSeries::testTime: time hour(" << hour << ") must be in range 0-23";
        throw std::runtime_error(ss.str());
    }
    if (minute < 0 || minute > 59) {
        std::stringstream ss;
        ss << "TimeSeries::testTime: time minute(" << minute << ") must be in range 0-59";
        throw std::runtime_error(ss.str());
    }
}

template <class Archive>
void TimeSeries::serialize(Archive& ar, std::uint32_t const /*version*/) {
    ar(CEREAL_NVP(start_));
    CEREAL_OPTIONAL_NVP(ar, finish_, [this]() { return !finish_.isNULL(); });
    CEREAL_OPTIONAL_NVP(ar, incr_, [this]() { return !incr_.isNULL(); });
    CEREAL_OPTIONAL_NVP(ar, nextTimeSlot_, [this]() { return !nextTimeSlot_.isNULL() && nextTimeSlot_ != start_; });
    CEREAL_OPTIONAL_NVP(
        ar, relativeDuration_, [this]() { return relativeDuration_ != boost::posix_time::time_duration(0, 0, 0, 0); });
    CEREAL_OPTIONAL_NVP(ar, relativeToSuiteStart_, [this]() { return relativeToSuiteStart_; });
    CEREAL_OPTIONAL_NVP(ar, isValid_, [this]() { return !isValid_; });

    if (Archive::is_loading::value) {
        if (nextTimeSlot_.isNULL()) {
            nextTimeSlot_ = start_;
        }
        if (!finish_.isNULL()) {
            compute_last_time_slot();
        }
    }
}
CEREAL_TEMPLATE_SPECIALIZE_V(TimeSeries);

} // namespace ecf
