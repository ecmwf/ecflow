/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/attribute/AutoArchiveAttr.hpp"

#include "ecflow/core/Calendar.hpp"
#include "ecflow/core/Converter.hpp"
#include "ecflow/core/Log.hpp"
#include "ecflow/core/Serialization.hpp"

#ifdef DEBUG
    #include <boost/date_time/posix_time/time_formatters.hpp>
#endif

using namespace std;
using namespace boost::gregorian;
using namespace boost::posix_time;

namespace ecf {

std::string AutoArchiveAttr::toString() const {
    std::string ret;
    write(ret);
    return ret;
}

void AutoArchiveAttr::write(std::string& ret) const {
    ret += "autoarchive ";
    if (days_) {
        ret += ecf::convert_to<std::string>(time_.hour() / 24);
        if (idle_)
            ret += " -i";
        return;
    }
    if (relative_)
        ret += "+";
    time_.print(ret);
    if (idle_)
        ret += " -i";
}

bool AutoArchiveAttr::operator==(const AutoArchiveAttr& rhs) const {
    if (relative_ != rhs.relative_)
        return false;
    if (days_ != rhs.days_)
        return false;
    if (idle_ != rhs.idle_)
        return false;
    return time_.operator==(rhs.time_);
}

bool AutoArchiveAttr::isFree(
    const ecf::Calendar& calendar,
    const std::pair<NState, boost::posix_time::time_duration>& last_state_and_change_duration) const {
    //                                                               suiteTime()
    //  suiteDurationAtComplete        autoarchive time              calendar duration
    //        |                             |                             |
    //        V                             V                             V
    // ----------------------------------------------------------------------------------> time
    //        ^                                                           ^
    //        |--------elapsed time---------------------------------------|
    //
    //

    bool is_valid_state = false;
    if (last_state_and_change_duration.first == NState::COMPLETE)
        is_valid_state = true;
    if (idle_) {
        if (last_state_and_change_duration.first == NState::QUEUED)
            is_valid_state = true;
        if (last_state_and_change_duration.first == NState::ABORTED)
            is_valid_state = true;
    }
    if (!is_valid_state) {
        return false;
    }

    if (relative_) {
        time_duration time_elapsed = calendar.duration() - last_state_and_change_duration.second;
        LOG_ASSERT(!time_elapsed.is_negative(), "should always be positive or some things gone wrong");
        if (time_elapsed >= time_.duration()) {
            return true;
        }
    }
    else {
        // real time
        // #ifdef DEBUG
        //     cout << "real time time_(" << to_simple_string(time_.duration())
        //          << ") calendar.suiteTime().time_of_day(" << to_simple_string(calendar.suiteTime().time_of_day()) <<
        //          ")\n";
        // #endif
        if (calendar.suiteTime().time_of_day() >= time_.duration()) {
            return true;
        }
    }

    return false;
}

template <class Archive>
void AutoArchiveAttr::serialize(Archive& ar, std::uint32_t const version) {
    ar(CEREAL_NVP(time_));
    CEREAL_OPTIONAL_NVP(ar, relative_, [this]() { return !relative_; }); // conditionally save
    CEREAL_OPTIONAL_NVP(ar, days_, [this]() { return days_; });          // conditionally save
    CEREAL_OPTIONAL_NVP(ar, idle_, [this]() { return idle_; });          // conditionally save
}
CEREAL_TEMPLATE_SPECIALIZE_V(AutoArchiveAttr);

} // namespace ecf
