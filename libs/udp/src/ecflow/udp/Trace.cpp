/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ecflow/udp/Trace.hpp"

#include <iostream>

namespace ecf::log {

Trace::Trace()
    : output_{std::cout},
      verbose_{false} {
}

void Trace::store(const std::string& entry) const {
    output_ << entry << std::endl;
}

Trace& getTrace() {
    static Trace instance;
    return instance;
}

} // namespace ecf::log
