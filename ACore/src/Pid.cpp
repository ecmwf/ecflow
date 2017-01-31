/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #11 $
//
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
// Description :
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8

#include "Pid.hpp"
#include <sys/types.h> // for getpid
#include <unistd.h>    // for getpid
#include <boost/lexical_cast.hpp>

std::string Pid::getpid()
{
   std::string pid;
   try { pid = boost::lexical_cast<std::string>(::getpid());  }
   catch (boost::bad_lexical_cast& e) {
      throw std::runtime_error("Pid::getpid(): Could not convert PID to a string\n");
   }
   return pid;
}

std::string Pid::unique_name(const std:: string& prefix)
{
   std::string ret = prefix;
   ret += "_";
   ret += Pid::getpid();
   return ret;
}
