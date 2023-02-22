/*
 * Copyright 2009-2023 ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "Trace.hpp"

#include <iostream>

namespace ecf::log {

Trace::Trace() : output_{std::cout}, verbose_{false} {
}

void Trace::store(const std::string& entry) const {
    output_ << entry << std::endl;
}

Trace& getTrace() {
    static Trace instance;
    return instance;
};

} // namespace ecf::log
