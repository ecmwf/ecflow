//============================================================================
// Copyright 2016 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "UiLog.hpp"

#include <iostream>
#include <assert.h>

#include "ServerHandler.hpp"
#include "TimeStamp.hpp"

UiLog::UiLog(ServerHandler* sh) :
    type_(INFO), server_(sh->longName())
{}

UiLog::UiLog(const std::string& server) :
    type_(INFO), server_(server)
{}


UiLog::~UiLog()
{
    output(os_.str());
}

void UiLog::appendType(std::string& s,Type t) const
{
    switch(t)
    {
    case DBG: s.append("DBG:"); break;
    case INFO: s.append("INF:"); break;
    case WARN: s.append("WAR:"); break;
    case ERROR: s.append("ERR:"); break;
    default: assert(false); break;
    }
}

std::ostringstream& UiLog::info()
{
    type_=INFO;
    return os_;
}

std::ostringstream& UiLog::err()
{
    type_=ERROR;
    return os_;
}

std::ostringstream& UiLog::warn()
{
    type_=WARN;
    return os_;
}

std::ostringstream& UiLog::dbg()
{
    type_=DBG;
    return os_;
}

void UiLog::output(const std::string& msg)
{
    std::string ts;
    //ecf::TimeStamp::now(ts);
    timeStamp(ts);

    std::string s;
    appendType(s,type_);

    if(server_.empty())
        s+=" " + ts + msg;
    else
        s+=" " + ts + "[" + server_ + "] " + msg;

    std::cout << s << std::endl;

}

void UiLog::timeStamp(std::string& time_stamp)
{
   char t_fmt[255];
   time_t stamp = time( NULL);
   struct tm *tod = localtime(&stamp);
   sprintf(t_fmt, "[%02d:%02d:%02d %d.%d] ", tod->tm_hour, tod->tm_min, tod->tm_sec,
           tod->tm_mday, tod->tm_mon + 1);

   time_stamp = t_fmt;
}

