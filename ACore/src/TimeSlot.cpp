/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
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
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8

#include "TimeSlot.hpp"
#include "Calendar.hpp"
#include "Str.hpp"

using namespace boost::posix_time;

namespace ecf {

///////////////////////////////////////////////////////////////////////////////////////////

bool TimeSlot::operator<(const TimeSlot& rhs) const
{
   if (hour_ <  rhs.hour()) return true;
   if (hour_ == rhs.hour()) {
      return minute_ < rhs.minute();
   }
   return false;
}

bool TimeSlot::operator>(const TimeSlot& rhs) const
{
   if (hour_ >  rhs.hour()) return true;
   if (hour_ == rhs.hour()) {
      return minute_ > rhs.minute();
   }
   return false;
}

bool TimeSlot::operator<=( const TimeSlot& rhs ) const
{
   if (operator<(rhs)) return true;
   return operator==(rhs);
}


bool TimeSlot::operator>=( const TimeSlot& rhs ) const
{
   if (operator>(rhs)) return true;
   return operator==(rhs);
}


std::ostream& TimeSlot::print(std::ostream& os) const
{
	os << toString();
 	return os;
}

std::string TimeSlot::toString() const
{
	std::stringstream ss;
	if (hour_ < 10) ss << "0" << hour_;
	else            ss << hour_;

	ss << Str::COLON();
	if (minute_ < 10) ss << "0" << minute_;
	else              ss << minute_;
	return ss.str();
}

boost::posix_time::time_duration TimeSlot::duration() const
{
	assert(!isNULL());
	return boost::posix_time::time_duration( hours(hour_) + minutes(minute_) ) ;
}


std::ostream& operator<<(std::ostream& os, const TimeSlot* d) {
	if (d) return d->print(os);
	return os << "TimeSlot == NULL";
}
std::ostream& operator<<(std::ostream& os, const TimeSlot& d)  { return d.print(os); }

}
