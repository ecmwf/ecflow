//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #20 $ 
//
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description :
//============================================================================

#include <cassert>
#include "NState.hpp"
#include "Ecf.hpp"
#include "Serialization.hpp"

void NState::setState( State s ) {
	st_= s;
	state_change_no_ = Ecf::incr_state_change_no();

#ifdef DEBUG_STATE_CHANGE_NO
	std::cout << "NState::setState\n";
#endif
}

const char* NState::toString( NState::State s ) {
	switch ( s ) {
		case NState::UNKNOWN:
			return "unknown";
			break;
		case NState::COMPLETE:
			return "complete";
			break;
		case NState::QUEUED:
			return "queued";
			break;
		case NState::ABORTED:
			return "aborted";
			break;
		case NState::SUBMITTED:
			return "submitted";
			break;
		case NState::ACTIVE:
			return "active";
			break;
		default:
			assert(false); break;
	}
	assert(false);
	return nullptr;
}


const char* NState::to_html( NState::State s ) {
   switch ( s ) {
      case NState::UNKNOWN:
         return "<state>unknown</state>";
         break;
      case NState::COMPLETE:
         return "<state>complete</state>";
         break;
      case NState::QUEUED:
         return "<state>queued</state>";
         break;
      case NState::ABORTED:
         return "<state>aborted</state>";
         break;
      case NState::SUBMITTED:
         return "<state>submitted</state>";
         break;
      case NState::ACTIVE:
         return "<state>active</state>";
         break;
      default:
         assert(false); break;
   }
   assert(false);
   return nullptr;
}

NState::State NState::toState( const std::string& str ) {
	if ( str == "complete" )
		return NState::COMPLETE;
	if ( str == "unknown" )
		return NState::UNKNOWN;
	if ( str == "queued" )
		return NState::QUEUED;
	if ( str == "aborted" )
		return NState::ABORTED;
	if ( str == "submitted" )
		return NState::SUBMITTED;
	if ( str == "active" )
		return NState::ACTIVE;
	assert(false);
	return NState::UNKNOWN;
}

bool NState::isValid( const std::string& state ) {
	if ( state == "complete" )
		return true;
	if ( state == "unknown" )
		return true;
	if ( state == "queued" )
		return true;
	if ( state == "aborted" )
		return true;
	if ( state == "submitted" )
		return true;
	if ( state == "active" )
		return true;
	return false;
}

std::vector< std::string > NState::allStates() {
	std::vector< std::string > vec;
	vec.reserve( 6 );
	vec.emplace_back("complete" );
	vec.emplace_back("unknown" );
	vec.emplace_back("queued" );
	vec.emplace_back("aborted" );
	vec.emplace_back("submitted" );
 	vec.emplace_back("active" );
	return vec;
}

std::vector<NState::State> NState::states()
{
	std::vector< NState::State > vec;
	vec.reserve( 6 );
	vec.push_back( NState::UNKNOWN    );
	vec.push_back( NState::COMPLETE   );
	vec.push_back( NState::QUEUED     );
	vec.push_back( NState::ABORTED    );
	vec.push_back( NState::SUBMITTED  );
	vec.push_back( NState::ACTIVE     );
 	return vec;
}


template<class Archive>
void NState::serialize(Archive & ar)
{
   ar(CEREAL_NVP(st_));
}
CEREAL_TEMPLATE_SPECIALIZE(NState);

