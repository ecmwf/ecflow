//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #32 $ 
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
#include <sstream>
#include <ostream>

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

void DayAttr::calendarChanged( const ecf::Calendar& c )
{
   // See ECFLOW-337
   if (c.dayChanged()) {
      clearFree();
   }
   if (free_) {
      return;
   }
   else if (isFree(c)) {
      setFree();
   }
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

bool DayAttr::checkForRequeue( const ecf::Calendar& calendar) const
{
 	// if calendar is hybrid, we can't requeue
	if (calendar.hybrid()) {
		return false;
	}

	// checkForRequeue is called when we are deciding whether to re-queue the node.
	// If this date is in the future, they we should re-queue
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
      if (free_) os += " # free";
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
	else           ss << " (holding)";
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

DayAttr::Day_t DayAttr::getDay(const std::string& day)
{
	if (day == "monday")         return DayAttr::MONDAY;
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
