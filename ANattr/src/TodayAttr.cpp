//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #38 $ 
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

#include "TodayAttr.hpp"
#include "Calendar.hpp"
#include "Indentor.hpp"
#include "Log.hpp"
#include "PrintStyle.hpp"
#include "Ecf.hpp"

using namespace std;

namespace ecf {

std::ostream& TodayAttr::print(std::ostream& os) const
{
   Indentor in;
   Indentor::indent(os) << toString();
   if (!PrintStyle::defsStyle()) {
      os << timeSeries_.state_to_string(makeFree_);
   }
   os << "\n";
   return os;
}

std::string TodayAttr::toString() const
{
   std::string ret = "today ";
   ret += timeSeries_.toString();
   return ret;
}

std::string TodayAttr::dump() const
{
	std::stringstream ss;
	ss << "today ";

    if (PrintStyle::getStyle() == PrintStyle::STATE) {
    	if (makeFree_) ss << "(free) ";
    	else           ss << "(holding) ";
    }

 	ss << timeSeries_.toString();

 	return ss.str();
}

bool TodayAttr::operator==(const TodayAttr& rhs) const
{
	if (makeFree_ != rhs.makeFree_) {
		return false;
	}
	return timeSeries_.operator==(rhs.timeSeries_);
}

bool TodayAttr::structureEquals(const TodayAttr& rhs) const
{
   return timeSeries_.structureEquals(rhs.timeSeries_);
}

void TodayAttr::miss_next_time_slot()
{
   timeSeries_.miss_next_time_slot();
   state_change_no_ = Ecf::incr_state_change_no();
}

void TodayAttr::setFree()
{
	makeFree_ = true;
	state_change_no_ =  Ecf::incr_state_change_no();
}

void TodayAttr::clearFree() {
	makeFree_ = false;
	state_change_no_ =  Ecf::incr_state_change_no();
}

void TodayAttr::calendarChanged( const ecf::Calendar& c )
{
   if ( makeFree_ ) {
      return;
   }

   if (timeSeries_.calendarChanged(c)) {
      state_change_no_ = Ecf::incr_state_change_no();
   }

   // For a time series, we rely on the re queue to reset makeFree
   if (isFree(c)) {
      setFree();
   }
}

void TodayAttr::resetRelativeDuration() {
   if (timeSeries_.resetRelativeDuration()) {
      state_change_no_ = Ecf::incr_state_change_no();
   }
}

bool TodayAttr::isFree(const ecf::Calendar& calendar) const
{
	// The FreeDepCmd can be used to free the today,
 	if (makeFree_) {
//		LOG(Log::DBG,"   TodayAttr::isFree makeFree_");
		return true;
	}
 	return is_free(calendar);
}

bool TodayAttr::is_free(const ecf::Calendar& calendar) const
{
   if (!timeSeries_.hasIncrement()) {
      if (timeSeries_.duration(calendar) > timeSeries_.start().duration() ) {
         return true;
      }
   }

   // For time series(/range), this  is already handle by timeSeries_
   // If timer expired return false. otherwise must match one time slot in the range/series
   return timeSeries_.isFree(calendar);
}

bool TodayAttr::why(const ecf::Calendar& c, std::string& theReasonWhy) const
{
   if (isFree(c)) return false;
   theReasonWhy += " is today dependent";

   // Check to see if time has expired, if has not, then report why
   if (timeSeries_.is_valid()) {
      // This can apply to single and series
      boost::posix_time::time_duration calendar_time = timeSeries_.duration(c);
      if (calendar_time < timeSeries_.start().duration()) {
         timeSeries_.why(c, theReasonWhy);
         return true;
      }

      // calendar_time >= timeSeries_.start().duration()
      if (timeSeries_.hasIncrement()) {
         if (calendar_time < timeSeries_.finish().duration()) {
            timeSeries_.why(c, theReasonWhy);
            return true;
         }
      }
      // calendar_time >= timeSeries_.start().duration() && calendar_time >= timeSeries_.finish().duration()
      // past the end of time slot, find next valid date
   }

   // the today has expired, report for the next day

   // For a single slot, we are always free after single slot time, hence we must look for tomorrow.
   // If we are in series then, we past the last time slot
   // Find the next date that matches
   boost::gregorian::date_duration one_day(1);
   boost::gregorian::date the_next_date = c.date();  // todays date
   the_next_date += one_day;                         // add one day, so its in the future

   theReasonWhy += " ( next run is at ";
   theReasonWhy += timeSeries_.start().toString();
   theReasonWhy += " ";
   theReasonWhy += to_simple_string( the_next_date );
   theReasonWhy += " )";
   return true;
}

}
