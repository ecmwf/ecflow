#ifndef TIMESTAMP_HPP_
#define TIMESTAMP_HPP_
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        : TimeStamp
// Author      : Avi
// Revision    : $Revision: #31 $
//
// Copyright 2009- ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8

#include <string>

namespace ecf {
namespace TimeStamp {

/// Generate a 'regular' time stamp.
///
/// Format specified as
///  - "[%02d:%02d:%02d %d.%d.%d] ", considering "[hour:min:sec day.month.year]
///  "
///
/// Results in the following examples
///  - "[05:26:20 29.10.2014] "
///  - "[05:26:20 17.1.2023] "
///
std::string now();
void now(std::string &);

/// Generate a 'brief' time stamp.
///
/// Format specified as
///  - "[%02d:%02d:%02d %d.%d] ", considering "[hour:min:sec day.month] "
///
/// Results in the following examples
///  - "[05:26:20 29.10] "
///  - "[05:26:20 17.1] "
///
void now_in_brief(std::string &);

} // namespace TimeStamp
} // namespace ecf

#endif
