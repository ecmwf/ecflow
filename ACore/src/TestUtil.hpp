/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_core_TestUtil_HPP
#define ecflow_core_TestUtil_HPP

///
/// \brief Test utility functions
///

#include <iterator>
#include <sstream>
#include <string>
#include <vector>

namespace ecf {

template <typename T>
static std::vector<std::string> toStrVec(const std::vector<T>& vec) {
    std::vector<std::string> retVec;
    retVec.reserve(vec.size());
    for (T s : vec) {
        retVec.push_back(s->name());
    }
    return retVec;
}

std::string toString(const std::vector<std::string>& c) {
    std::stringstream ss;
    std::copy(c.begin(), c.end(), std::ostream_iterator<std::string>(ss, ", "));
    return ss.str();
}

} // namespace ecf

#endif /* ecflow_core_TestUtil_HPP */
