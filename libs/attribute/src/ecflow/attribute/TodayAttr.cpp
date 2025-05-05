/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/attribute/TodayAttr.hpp"

#include <sstream>
#include <stdexcept>

#include "ecflow/attribute/DateAttr.hpp" // Used in Why
#include "ecflow/attribute/DayAttr.hpp"  // Used in Why
#include "ecflow/core/Calendar.hpp"
#include "ecflow/core/Ecf.hpp"
#include "ecflow/core/Indentor.hpp"
#include "ecflow/core/PrintStyle.hpp"
#include "ecflow/core/Serialization.hpp"
#include "ecflow/core/Str.hpp"

using namespace std;

namespace ecf {

TodayAttr::TodayAttr(const std::string& str) {
    if (str.empty())
        throw std::runtime_error("Today::Today: empty string passed");
    std::vector<std::string> tokens;
    Str::split(str, tokens);
    if (tokens.empty())
        throw std::runtime_error("Today::Today: incorrect time string ?");

    size_t index = 0;
    ts_          = TimeSeries::create(index, tokens, false /*parse_state*/);
}

void TodayAttr::print(std::string& os) const {
    Indentor in;
    Indentor::indent(os);
    write(os);
    if (!PrintStyle::defsStyle()) {
        ts_.write_state(os, free_);
    }
    os += "\n";
}

std::string TodayAttr::name() const {
    std::string ret;
    write(ret);
    ts_.write_state_for_gui(ret, free_);
    return ret;
}

std::string TodayAttr::toString() const {
    std::string ret;
    write(ret);
    return ret;
}

void TodayAttr::write(std::string& ret) const {
    ret += "today ";
    ts_.write(ret);
}

std::string TodayAttr::dump() const {
    std::stringstream ss;
    ss << "today ";

    if (PrintStyle::getStyle() == PrintStyle::STATE) {
        if (free_)
            ss << "(free) ";
        else
            ss << "(holding) ";
    }

    ss << ts_.dump();

    return ss.str();
}

bool TodayAttr::operator==(const TodayAttr& rhs) const {
    if (free_ != rhs.free_) {
        return false;
    }
    return ts_.operator==(rhs.ts_);
}

bool TodayAttr::structureEquals(const TodayAttr& rhs) const {
    return ts_.structureEquals(rhs.ts_);
}

void TodayAttr::miss_next_time_slot() {
    ts_.miss_next_time_slot();
    state_change_no_ = Ecf::incr_state_change_no();
}

void TodayAttr::setFree() {
    free_            = true;
    state_change_no_ = Ecf::incr_state_change_no();
}

void TodayAttr::clearFree() {
    free_            = false;
    state_change_no_ = Ecf::incr_state_change_no();
}

void TodayAttr::calendarChanged(const ecf::Calendar& c) {
    // ensure this called first , since we need always update for relative duration ECFLOW-1648
    if (ts_.calendarChanged(c)) {
        state_change_no_ = Ecf::incr_state_change_no();
    }

    if (free_) {
        return;
    }

    // For a time series, we rely on the re queue to reset makeFree
    if (isFree(c)) {
        setFree();
    }
}

void TodayAttr::resetRelativeDuration() {
    if (ts_.resetRelativeDuration()) {
        state_change_no_ = Ecf::incr_state_change_no();
    }
}

bool TodayAttr::isFree(const ecf::Calendar& calendar) const {
    // The FreeDepCmd can be used to free the today,
    if (free_) {
        return true;
    }
    return is_free(calendar);
}

bool TodayAttr::is_free(const ecf::Calendar& calendar) const {
    if (!ts_.hasIncrement()) {
        if (ts_.duration(calendar) > ts_.start().duration()) {
            return true;
        }
    }

    // For time series(/range), this  is already handle by ts_
    // If timer expired return false. otherwise must match one time slot in the range/series
    return ts_.isFree(calendar);
}

bool TodayAttr::why(const ecf::Calendar& c,
                    const std::vector<DayAttr>& days,
                    const std::vector<DateAttr>& dates,
                    std::string& theReasonWhy) const {
    if (isFree(c))
        return false;
    theReasonWhy += "is today ";
    if (!days.empty())
        theReasonWhy += "and day ";
    if (!dates.empty())
        theReasonWhy += "and date ";
    theReasonWhy += "dependent";

    // Check to see if time has expired; if not, then report why
    if (ts_.is_valid()) {
        // This can apply to single and series
        boost::posix_time::time_duration calendar_time = ts_.duration(c);
        if (calendar_time < ts_.start().duration()) {
            ts_.why(c, theReasonWhy);
            return true;
        }

        // calendar_time >= ts_.start().duration()
        if (ts_.hasIncrement()) {
            if (calendar_time < ts_.finish().duration()) {
                ts_.why(c, theReasonWhy);
                return true;
            }
        }
        // calendar_time >= ts_.start().duration() && calendar_time >= ts_.finish().duration()
        // past the end of time slot, hence this should not hold job generation,
    }

    // the today has expired,
    theReasonWhy += " ( '";
    theReasonWhy += toString();
    theReasonWhy += "' has expired,";

    // take into account, user can use run/force complete to miss time slots
    bool do_a_requeue = ts_.requeueable(c);
    if (do_a_requeue) {
        TimeSlot the_next_time_slot = ts_.compute_next_time_slot(c);
        if (the_next_time_slot.isNULL() || !ts_.hasIncrement()) {
            theReasonWhy += " *re-queue* to run at this time";
        }
        else {
            theReasonWhy += " *re-queue* to run at";
            theReasonWhy += the_next_time_slot.toString();
        }
    }
    else {
        if (ts_.relative()) {
            theReasonWhy += " please *re-queue*, to reset the relative duration";
        }
        else {
            boost::gregorian::date the_min_next_date;

            if (!days.empty() || !dates.empty()) {
                for (auto day : days) {
                    boost::gregorian::date the_next_matching_date = day.next_matching_date(c);
                    if (the_min_next_date.is_special())
                        the_min_next_date = the_next_matching_date;
                    if (the_next_matching_date < the_min_next_date)
                        the_min_next_date = the_next_matching_date;
                }
                for (const auto& i : dates) {
                    boost::gregorian::date the_next_matching_date = i.next_matching_date(c);
                    if (the_min_next_date.is_special())
                        the_min_next_date = the_next_matching_date;
                    if (the_next_matching_date < the_min_next_date)
                        the_min_next_date = the_next_matching_date;
                }

                theReasonWhy += " next run at ";
            }
            else {
                boost::gregorian::date_duration one_day(1);
                the_min_next_date = c.date(); // today's date
                the_min_next_date += one_day; // add one day, so its in the future
                theReasonWhy += " next run tomorrow at ";
            }

            theReasonWhy += ts_.start().toString();
            theReasonWhy += " ";
            theReasonWhy += to_simple_string(the_min_next_date);
        }
    }
    theReasonWhy += " )";

    return true;
}

template <class Archive>
void TodayAttr::serialize(Archive& ar) {
    ar(CEREAL_NVP(ts_));
    CEREAL_OPTIONAL_NVP(ar, free_, [this]() { return free_; }); // conditionally save
}
CEREAL_TEMPLATE_SPECIALIZE(TodayAttr);

} // namespace ecf
