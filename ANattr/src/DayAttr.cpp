//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #32 $ 
//
// Copyright 2009-2020 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description :
//============================================================================
#include <cassert>
#include <sstream>

#include "DayAttr.hpp"
#include "Indentor.hpp"
#include "Calendar.hpp"
#include "PrintStyle.hpp"
#include "Ecf.hpp"
#include "Serialization.hpp"

using namespace std;
using namespace ecf;

//===============================================================================

static const char* theDay(DayAttr::Day_t day)
{
   switch (day) {
      case DayAttr::SUNDAY:    return "sunday"; break;
      case DayAttr::MONDAY:    return "monday"; break;
      case DayAttr::TUESDAY:   return "tuesday"; break;
      case DayAttr::WEDNESDAY: return "wednesday"; break;
      case DayAttr::THURSDAY:  return "thursday"; break;
      case DayAttr::FRIDAY:    return "friday"; break;
      case DayAttr::SATURDAY:  return "saturday"; break;
      default: assert(false);break;
   }
   return nullptr;
}

//===============================================================================

void DayAttr::calendarChanged( const ecf::Calendar& c, bool clear_at_midnight)
{
	// See ECFLOW-337
	//	repeat ....
	//    family start
	//      family 0
	//        time 10:00
	//        day monday        # if there was no c.dayChanged(), then after re-queue, & before midnight Monday is still free.
	//        task dummy        # hence we will end up also running on Tuesday at 10:00
	//          complete 1==1
	//          trigger 0==1
	//
	//  ECFLOW-1550            # If children of a family with day/date are still active/submitted/queued, then don't clear at midnight.
	//  repeat ....            #  This is only applicable for NodeContainers, for task with day/date always CLEAR at midnight
	//	family f1
	//	   day monday
	//	   time 23:00
	//	   task t1             # Task t1 took longer than 1 hour
	//	   task t2             # allow task t2 to continue to the next day, i.e clear_at_midnight = False
	//	      trigger t1 == complete

	if (c.dayChanged()) {
		if (clear_at_midnight) clearFree();
	}

	if (free_) {
		return;
	}

	if (is_free(c)) {
		setFree();
	}
}

void DayAttr::reset()
{
   free_ = false;
   state_change_no_ = Ecf::incr_state_change_no();
}

void DayAttr::requeue()
{
   free_ = false;
   state_change_no_ = Ecf::incr_state_change_no();
}

bool DayAttr::isFree(const ecf::Calendar& calendar) const
{
	// The FreeDepCmd can be used to free the dates,
 	if (free_) {
		return true;
	}
 	return is_free(calendar);
}

bool DayAttr::is_free(const ecf::Calendar& calendar) const
{
   return (calendar.day_of_week() == day_);
}

void DayAttr::setFree() {
	free_ = true;
	state_change_no_ = Ecf::incr_state_change_no();

#ifdef DEBUG_STATE_CHANGE_NO
	std::cout << "DayAttr::setFree()\n";
#endif
}

void DayAttr::clearFree() {
	free_ = false;
	state_change_no_ = Ecf::incr_state_change_no();

#ifdef DEBUG_STATE_CHANGE_NO
	std::cout << "DayAttr::clearFree()\n";
#endif
}

bool DayAttr::checkForRequeue( const ecf::Calendar& calendar,const std::vector<DayAttr>& days) const
{
 	// if calendar is hybrid, we can't requeue
	if (calendar.hybrid()) {
		return false;
	}

	// checkForRequeue is called when we are deciding whether to re-queue the node.(under AUTOMATED RE-QUEUE)
	// Hence we *MUST* have completed with *THIS* day. Also crons,time,today have all returned false.
	// If this date is in the future, they we should re-queue
	// 	    return (day_ > calendar.day_of_week() );
	//    sunday    0
	//    monday    1
	//    tuesday   2
	//    wednesday 3
	//    thursday  4
	//    friday    5
	//    saturday  6
	//  *HOWEVER* this breaks if day is Saturday(6) and next day is Sunday(0) *or* any other day,
	//  or put another way, this will always re-queue for Saturday, irrespective of future day.
	//    task t1
	//       time 23:00
	//       day  saturday
	//       day  monday
	//  This becomes more problematic when we have top level repeat, as it will never complete, i.e ECFLOW-1628
	//  ECFLOW-1628
	//  repeat ....
	//	  family f1
	//	    day monday
	//	    time 23:00
	//	    task t1
	//	  family f1
	//	    day saturday  # using   return (day_ > calendar.day_of_week() ); means checkForRequeue() will always true for Saturday
	//	    time 23:00
	//	    task t1
	//
	// Likewise:
	//	suite ecflow_1628
	//	  clock    real 20.04.2020     # Monday
	//	  endclock      27.04.2020     # Monday, add endclock otherwise we simulate for year due to repeat.
	//	  family fam
	//	    verify complete:2 
	//	    day sunday
	//	    day tuesday
	//	    time 08:00
	//	    task t1
	//	        verify complete:2        # task should complete twice
	// This will ONLY complete once, because sunday(0) will *NEVER* satisfy (day_ > calendar.day_of_week()) <TODO>

	// The *NORMAL* use case is to have a single day, hence we will return false, when ever we have a single day.
	if (days.size() == 1) {
		// just have a single day, which *MUST* be this day
		return false; // Handle the single Saturday under a repeat loop.
	}

	return (day_ > calendar.day_of_week() );
}

bool DayAttr::validForHybrid(const ecf::Calendar& calendar) const
{
 	return isFree(calendar);
}

bool DayAttr::why(const ecf::Calendar& c, std::string& theReasonWhy) const
{
	if (isFree(c))  return false;

	theReasonWhy += " is day dependent ( next run on ";
	theReasonWhy += theDay(day_);
	theReasonWhy += " the current day is ";
	theReasonWhy += theDay(static_cast<DayAttr::Day_t>(c.day_of_week()));
	theReasonWhy += " )";
 	return true;
}

void DayAttr::print(std::string& os) const
{
	Indentor in;
	Indentor::indent(os) ; write(os);
   if (!PrintStyle::defsStyle()) {
      if (free_) {
         os += " # free";
      }
   }
	os += "\n";
}

std::string DayAttr::toString() const
{
   std::string ret;
   write(ret);
   return ret;
}

void DayAttr::write(std::string& ret) const
{
   ret += "day ";
   ret += theDay(day_);
}

std::string DayAttr::dump() const
{
	std::stringstream ss;
	ss << toString();
 	if (free_) ss << " (free)";
	else       ss << " (holding)";
  	return ss.str();
}

bool DayAttr::operator==(const DayAttr& rhs) const
{
	if (free_ != rhs.free_) {
		return false;
	}
	return structureEquals(rhs);
}

bool DayAttr::structureEquals(const DayAttr& rhs) const
{
	return (day_ == rhs.day_);
}


DayAttr DayAttr::create(const std::string& dayStr)
{
   return DayAttr( getDay(dayStr) );
}

DayAttr DayAttr::create( const std::vector<std::string>& lineTokens, bool read_state)
{
   if ( lineTokens.size() < 2 ) {
      throw std::runtime_error( "DayAttr::create date tokens to short :");
   }

//   for(size_t i =0; i < lineTokens.size() ; i++) {
//      cout << "lineTokens[" << i << "] = '" << lineTokens[i] << "'\n";
//   }

   // day monday  # free
   DayAttr day = DayAttr::create( lineTokens[1] );

   // state
   if (read_state) {
      for(size_t i = 3; i < lineTokens.size(); i++) {
         if (lineTokens[i] == "free") day.setFree();
      }
   }
   return day;
}

DayAttr::Day_t DayAttr::getDay(const std::string& day)
{
	if (day == "monday")    return DayAttr::MONDAY;
	if (day == "tuesday")   return DayAttr::TUESDAY;
	if (day == "wednesday") return DayAttr::WEDNESDAY;
	if (day == "thursday")  return DayAttr::THURSDAY;
	if (day == "friday")    return DayAttr::FRIDAY;
	if (day == "saturday")  return DayAttr::SATURDAY;
	if (day == "sunday")    return DayAttr::SUNDAY;

	std::stringstream ss;
	ss << "Invalid day(" << day << ") specification expected one of [monday,tuesday,wednesday,thursday,friday,saturday,sunday]: ";
	throw std::runtime_error(ss.str());

	return DayAttr::SUNDAY;
}

std::vector< std::string > DayAttr::allDays() {
	std::vector< std::string > vec;
	vec.reserve( 7 );
	vec.emplace_back("monday");
	vec.emplace_back("tuesday");
	vec.emplace_back("wednesday");
	vec.emplace_back("thursday");
	vec.emplace_back("friday");
	vec.emplace_back("saturday");
	vec.emplace_back("sunday");
	return vec;
}

boost::gregorian::date DayAttr::next_matching_date(const ecf::Calendar& c) const
{
    boost::gregorian::date_duration one_day(1);
    boost::gregorian::date the_next_matching_date = c.date();  // todays date

    for(int i=0; i<7; i++) {
    	the_next_matching_date += one_day;
    	if (the_next_matching_date.day_of_week().as_number() == day_) {
    		return the_next_matching_date;
    	}
    }
    assert(false);
    return c.date();
}

template<class Archive>
void DayAttr::serialize(Archive & ar )
{
   ar( CEREAL_NVP(day_));
   CEREAL_OPTIONAL_NVP(ar, free_, [this](){return free_;});  // conditionally save
}
CEREAL_TEMPLATE_SPECIALIZE(DayAttr);
