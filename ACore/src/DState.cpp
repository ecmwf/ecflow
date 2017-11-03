//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #10 $ 
//
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description :
//============================================================================
#include <assert.h>
#include <iostream>
#include "DState.hpp"
#include "Ecf.hpp"

void DState::setState( State s ) {
	state_= s;
	state_change_no_ = Ecf::incr_state_change_no();

#ifdef DEBUG_STATE_CHANGE_NO
	std::cout << "DState::setState\n";
#endif
}

NState::State DState::convert(DState::State display_state)
{
	switch (display_state) {
		case DState::UNKNOWN:  return NState::UNKNOWN; break;
		case DState::COMPLETE:  return NState::COMPLETE; break;
		case DState::SUSPENDED: return NState::UNKNOWN; break;
		case DState::QUEUED:    return NState::QUEUED; break;
		case DState::ABORTED:   return NState::ABORTED; break;
		case DState::SUBMITTED: return NState::SUBMITTED; break;
		case DState::ACTIVE:    return NState::ACTIVE; break;
	}
	return NState::UNKNOWN;
}

const char* DState::toString( DState::State s ) {
	switch ( s ) {
		case DState::UNKNOWN:
			return "unknown";
			break;
		case DState::COMPLETE:
			return "complete";
			break;
		case DState::QUEUED:
			return "queued";
			break;
		case DState::ABORTED:
			return "aborted";
			break;
		case DState::SUBMITTED:
			return "submitted";
			break;
		case DState::SUSPENDED:
			return "suspended";
			break;
		case DState::ACTIVE:
			return "active";
			break;
		default:
			assert(false);break;
	}
	assert(false);
	return NULL;
}

const char* DState::to_html( DState::State s ) {
   switch ( s ) {
      case DState::UNKNOWN:
         return "<state>unknown</state>";
         break;
      case DState::COMPLETE:
         return "<state>complete</state>";
         break;
      case DState::QUEUED:
         return "<state>queued</state>";
         break;
      case DState::ABORTED:
         return "<state>aborted</state>";
         break;
      case DState::SUBMITTED:
         return "<state>submitted</state>";
         break;
      case DState::SUSPENDED:
         return "<state>suspended</state>";
         break;
      case DState::ACTIVE:
         return "<state>active</state>";
         break;
      default:
         assert(false);break;
   }
   assert(false);
   return NULL;
}

DState::State DState::toState( const std::string& str ) {
	if ( str == "complete" )
		return DState::COMPLETE;
	if ( str == "unknown" )
		return DState::UNKNOWN;
	if ( str == "queued" )
		return DState::QUEUED;
	if ( str == "aborted" )
		return DState::ABORTED;
	if ( str == "submitted" )
		return DState::SUBMITTED;
	if ( str == "suspended" )
		return DState::SUSPENDED;
	if ( str == "active" )
		return DState::ACTIVE;
	throw std::runtime_error("DState::toState: Can change string to a DState :"+ str);
	return DState::UNKNOWN;
}

bool DState::isValid( const std::string& state ) {
	if ( state == "complete" )
		return true;
   if ( state == "aborted" )
      return true;
   if ( state == "queued" )
      return true;
   if ( state == "active" )
      return true;
	if ( state == "suspended" )
		return true;
	if ( state == "unknown" )
		return true;
	if ( state == "submitted" )
		return true;
	return false;
}

std::vector< std::string > DState::allStates() {
	std::vector< std::string > vec;
	vec.reserve( 7 );
	vec.push_back( "complete" );
	vec.push_back( "unknown" );
	vec.push_back( "queued" );
	vec.push_back( "aborted" );
	vec.push_back( "submitted" );
	vec.push_back( "suspended" );
	vec.push_back( "active" );
	return vec;
}

std::vector<DState::State> DState::states()
{
	std::vector< DState::State > vec;
	vec.reserve( 7 );
	vec.push_back( DState::UNKNOWN    );
	vec.push_back( DState::COMPLETE   );
	vec.push_back( DState::QUEUED     );
	vec.push_back( DState::ABORTED    );
	vec.push_back( DState::SUBMITTED  );
	vec.push_back( DState::ACTIVE     );
	vec.push_back( DState::SUSPENDED  );
	return vec;
}

