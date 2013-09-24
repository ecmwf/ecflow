/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
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
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8

#include "SState.hpp"
#include <assert.h>

std::string SState::to_string(int status)
{
	if (status == 0)      return "HALTED";
	else if (status == 1) return "SHUTDOWN";
	else if (status == 2) return "RUNNING";
	return "UNKNOWN??";
}

std::string SState::to_string(SState::State state)
{
	switch (state) {
		case SState::HALTED: return "HALTED"; break;
		case SState::SHUTDOWN: return "SHUTDOWN"; break;
		case SState::RUNNING: return "RUNNING"; break;
	}
	return "UNKNOWN??";
}

SState::State SState::toState( const std::string& str ) {
   if ( str == "HALTED" )
      return SState::HALTED;
   if ( str == "SHUTDOWN" )
      return SState::SHUTDOWN;
   if ( str == "RUNNING" )
      return SState::RUNNING;
   assert(false);
   return SState::HALTED;
}

bool SState::isValid( const std::string& state ) {
   if ( state == "HALTED" )
      return true;
   if ( state == "SHUTDOWN" )
      return true;
   if ( state == "queued" )
      return true;
   if ( state == "RUNNING" )
      return true;
   return false;
}
