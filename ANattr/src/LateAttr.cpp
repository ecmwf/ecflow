//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #16 $ 
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
#include <sstream>
#include <boost/foreach.hpp>

#include "LateAttr.hpp"
#include "Indentor.hpp"
#include "Calendar.hpp"
#include "Ecf.hpp"
#include "PrintStyle.hpp"

using namespace std;
using namespace boost::gregorian;
using namespace boost::posix_time;

namespace ecf {

LateAttr::LateAttr() :  completeIsRelative_(false),isLate_(false),state_change_no_(0) {}

std::ostream& LateAttr::print(std::ostream& os) const
{
	Indentor in;
	Indentor::indent(os) << toString();
   if (!PrintStyle::defsStyle()) {
      if (isLate_) os << " # late";
   }
   os << "\n";
   return os;
}

std::string LateAttr::toString() const
{
   std::string ret = "late";
   if (!submitted_.isNULL()) {
      ret += " -s +";
      ret += submitted_.toString();
   }
   if (!active_.isNULL()) {
      ret += " -a ";
      ret += active_.toString();
   }
   if (!complete_.isNULL()) {
      ret += " -c ";
      if (completeIsRelative_) ret += "+";
      ret += complete_.toString();
   }
   return ret;
}

bool LateAttr::operator==(const LateAttr& rhs) const
{
 	if ( completeIsRelative_ != rhs.completeIsRelative_) return false;
	if ( submitted_ != rhs.submitted_) return false;
	if ( active_ != rhs.active_) return false;
   if ( complete_ != rhs.complete_) return false;
   if ( isLate_ != rhs.isLate_) return false;
	return true;
}

bool LateAttr::isNull() const
{
	return ( submitted_.isNULL() && active_.isNULL() && complete_.isNULL());
}

void LateAttr::checkForLateness( const std::pair<NState,boost::posix_time::time_duration>&  state,
                                 const ecf::Calendar& calendar )
{
	if (isLate_ || isNull()) {
		return;
	}

	if (state.first == NState::SUBMITTED || state.first == NState::QUEUED) {
		// Submitted is always relative, ASSUME this means relative to suite start
		if (state.first == NState::SUBMITTED && !submitted_.isNULL()) {
			if ( calendar.duration() > submitted_.duration()  ) {
	 			setLate(true);
 				return;
			}
		}

		// In Submitted or queued state, check for active, in REAL time
 		if (!active_.isNULL() && calendar.suiteTime().time_of_day() > active_.duration()) {
 			setLate(true);
		}
 	}
	else if ( state.first == NState::ACTIVE && !complete_.isNULL()) {
		if ( completeIsRelative_) {
			// to check for complete, we need the duration when state went into active state
			// state.second is when state went ACTIVE, relative to suite start
 			boost::posix_time::time_duration runtime =  calendar.duration() - state.second ;
			if ( runtime > complete_.duration()) {
				setLate(true);
 			}
		}
		else {
			// Real time
 			if (calendar.suiteTime().time_of_day() > complete_.duration()) {
 				setLate(true);
			}
		}
	}
}

void LateAttr::setLate(bool f)
{
	isLate_ = f;
	state_change_no_ = Ecf::incr_state_change_no();

#ifdef DEBUG_STATE_CHANGE_NO
	std::cout << "LateAttr::setLate\n";
#endif
}

}
