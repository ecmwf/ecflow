/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_core_AssertTimer_HPP
#define ecflow_core_AssertTimer_HPP

///
/// \brief Simple class that asserts when a time constraint is not met
///

#include "ecflow/core/Calendar.hpp"

namespace ecf {

// Local timer class used to ensure a job submission takes less than 60 seconds
class AssertTimer {
public:
    explicit AssertTimer(int t, bool doAssert = true)
        : timeConstraint_(t),
          doAssert_(doAssert),
          start_time_(Calendar::second_clock_time()) {}
    ~AssertTimer();

    int timeConstraint() const { return timeConstraint_; }

    int duration() const {
        auto duration = Calendar::second_clock_time() - start_time_;
        return duration.total_seconds();
    }

private:
    int timeConstraint_;
    bool doAssert_;
    boost::posix_time::ptime start_time_;
};

} // namespace ecf

#endif /* ecflow_core_AssertTimer_HPP */
