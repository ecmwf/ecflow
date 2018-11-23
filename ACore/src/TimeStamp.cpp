//============================================================================
// Name        : Log
// Author      : Avi
// Revision    : $Revision: #57 $
//
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
// Description : Simple singleton implementation of log
//============================================================================
#include "TimeStamp.hpp"
#include <cstdio>
#include <ctime>

using namespace std;

namespace ecf {

std::string TimeStamp::now()
{
   std::string time_stamp;
   now(time_stamp);
   return time_stamp;
}

void TimeStamp::now(std::string& time_stamp)
{
   char t_fmt[255];
   time_t stamp = std::time( nullptr);
   struct tm *tod = localtime(&stamp);   // cppcheck-suppress localtimeCalled
   sprintf(t_fmt, "[%02d:%02d:%02d %d.%d.%d] ", tod->tm_hour, tod->tm_min, tod->tm_sec,
           tod->tm_mday, tod->tm_mon + 1, tod->tm_year + 1900);

   time_stamp = t_fmt;
}

void TimeStamp::now_in_brief(std::string& time_stamp)
{
   char t_fmt[255];
   time_t stamp = time( nullptr);
   struct tm *tod = localtime(&stamp);  // cppcheck-suppress localtimeCalled
   sprintf(t_fmt, "[%02d:%02d:%02d %d.%d] ", tod->tm_hour, tod->tm_min, tod->tm_sec,
           tod->tm_mday, tod->tm_mon + 1);

   time_stamp = t_fmt;
}
}
