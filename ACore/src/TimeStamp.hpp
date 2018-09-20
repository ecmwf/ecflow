#ifndef TIMESTAMP_HPP_
#define TIMESTAMP_HPP_
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        : Log
// Author      : Avi
// Revision    : $Revision: #31 $
//
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8

#include <string>

namespace ecf {

// returns a string of format : "[%02d:%02d:%02d %d.%d.%d] "
//                              "[hour:min:sec day.month.year] "
// i.e                          "[05:26:20 29.10.2014] "
class TimeStamp {
public:
   static std::string now();
   static void now(std::string&);
   static void now_in_brief(std::string&);
private:
   TimeStamp() = delete;
   TimeStamp(const TimeStamp&) = delete;
   const TimeStamp& operator=(const TimeStamp&) = delete;
};

}

#endif
