/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "AssertTimer.hpp"

#include <iostream>

#include "Log.hpp"

namespace ecf {

AssertTimer::~AssertTimer() {
    if (doAssert_ && timeConstraint_ > 0) {
        int d = duration();
        if (d >= timeConstraint_) {
            std::cout << "AssertTimer::~AssertTimer() duration(" << d << ") >= timeConstraint(" << timeConstraint_
                      << ")\n";
        }
        LOG_ASSERT(d < timeConstraint_, "AssertTimer::~AssertTimer()");
    }
}

} // namespace ecf
