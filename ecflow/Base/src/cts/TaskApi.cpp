//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #4 $ 
//
// Copyright 2009-2012 ECMWF. 
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description :
//============================================================================
#include <boost/algorithm/string/trim.hpp>
#include "TaskApi.hpp"

std::string TaskApi::init(const std::string& process_id)
{
   std::string ret = "--init=";
   ret += process_id;
   return ret;
}
const char* TaskApi::initArg()      { return "init"; }


std::string TaskApi::abort(const std::string& reason)
{
   if (reason.empty()) return "--abort";
   std::string ret = "--abort=";
   ret += reason;
   return ret;
}
const char* TaskApi::abortArg()     { return "abort"; }


std::string TaskApi::event(const std::string& eventName) {
   std::string ret = "--event=";
   ret += eventName;
   return ret;
}
const char* TaskApi::eventArg()     { return "event"; }


std::vector<std::string> TaskApi::meter(const std::string& meterName,const std::string& new_meter_value)
{
   std::vector<std::string> retVec; retVec.reserve(2);
   std::string ret = "--meter=";ret += meterName;
   retVec.push_back(ret);
   retVec.push_back(new_meter_value);
   return retVec;
}
const char* TaskApi::meterArg()     { return "meter"; }


std::vector<std::string> TaskApi::label(const std::string& label_name, const std::vector<std::string>& labels )
{
   std::vector<std::string> retVec; retVec.reserve(labels.size() +1);
   std::string ret = "--label=";
   ret += label_name;
   retVec.push_back(ret);
   for(size_t i = 0; i < labels.size(); i++) { retVec.push_back(labels[i]);}
   return retVec;
}
const char* TaskApi::labelArg()     { return "label"; }


std::string TaskApi::complete()     { return "--complete"; }
const char* TaskApi::completeArg()  { return "complete"; }


std::string TaskApi::wait(const std::string& expression)
{
   std::string ret = "--wait=";
   ret += expression;
   return ret;
}
const char* TaskApi::waitArg()      { return "wait"; }


