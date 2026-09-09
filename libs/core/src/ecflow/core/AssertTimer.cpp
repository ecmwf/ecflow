/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ecflow/core/AssertTimer.hpp"

#include <iostream>

#include "ecflow/core/Log.hpp"

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
