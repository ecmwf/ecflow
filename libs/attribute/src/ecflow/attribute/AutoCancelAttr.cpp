/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ecflow/attribute/AutoCancelAttr.hpp"

#include "ecflow/core/Calendar.hpp"
#include "ecflow/core/Chrono.hpp"
#include "ecflow/core/Converter.hpp"
#include "ecflow/core/Log.hpp"
#include "ecflow/core/Serialization.hpp"

namespace ecf {

std::string AutoCancelAttr::toString() const {
    std::string ret;
    write(ret);
    return ret;
}

void AutoCancelAttr::write(std::string& ret) const {
    ret += "autocancel ";
    if (days_) {
        ret += ecf::convert_to<std::string>(time_.hour() / 24);
        return;
    }

    if (relative_) {
        ret += "+";
    }
    time_.print(ret);
}

bool AutoCancelAttr::operator==(const AutoCancelAttr& rhs) const {
    if (relative_ != rhs.relative_) {
        return false;
    }
    if (days_ != rhs.days_) {
        return false;
    }
    return time_.operator==(rhs.time_);
}

bool AutoCancelAttr::isFree(const ecf::Calendar& calendar,
                            const boost::posix_time::time_duration& suiteDurationAtComplete) const {
    //                                                               suiteTime()
    //  suiteDurationAtComplete        autocancel time               calendar duration
    //        |                             |                             |
    //        V                             V                             V
    // ----------------------------------------------------------------------------------> time
    //        ^                                                           ^
    //        |--------elapsed time---------------------------------------|
    //
    //

    if (relative_) {
        boost::posix_time::time_duration timeElapsedAfterComplete = calendar.duration() - suiteDurationAtComplete;
        LOG_ASSERT(!timeElapsedAfterComplete.is_negative(), "should always be positive or some things gone wrong");
        if (timeElapsedAfterComplete >= time_.duration()) {
            return true;
        }
    }
    else {
        // real time
        // #ifdef DEBUG
        //		cout << "real time time_(" << to_simple_string(time_.duration())
        //		     << ") calendar.suiteTime().time_of_day(" <<
        // to_simple_string(calendar.suiteTime().time_of_day()) << ")\n"; #endif
        if (calendar.suiteTime().time_of_day() >= time_.duration()) {
            return true;
        }
    }

    return false;
}

template <class Archive>
void AutoCancelAttr::serialize(Archive& ar, std::uint32_t const /*version*/) {
    ar(CEREAL_NVP(time_));
    CEREAL_OPTIONAL_NVP(ar, relative_, [this]() { return !relative_; }); // conditionally save
    CEREAL_OPTIONAL_NVP(ar, days_, [this]() { return days_; });          // conditionally save
}
CEREAL_TEMPLATE_SPECIALIZE_V(AutoCancelAttr);

} // namespace ecf
