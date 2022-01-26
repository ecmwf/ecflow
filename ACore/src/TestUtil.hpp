#ifndef TEST_UTIL_HPP_
#define TEST_UTIL_HPP_ 
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #57 $ 
//
// Copyright 2009- ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description : Test utility functions
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8

#include <vector>
#include <string>
#include <sstream>
#include <iterator>

namespace ecf {

    template <typename T>
    static std::vector<std::string> toStrVec(const std::vector<T>& vec)
    {
        std::vector<std::string> retVec; retVec.reserve(vec.size());
        for(T s: vec) { retVec.push_back(s->name()); }
        return retVec;
    }

    std::string toString(const std::vector<std::string>& c)
    {
        std::stringstream ss;
        std::copy(c.begin(), c.end(), std::ostream_iterator <std::string> (ss, ", "));
        return ss.str();
    }
};

#endif
