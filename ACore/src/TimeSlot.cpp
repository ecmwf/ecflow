/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #16 $ 
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

#include <ostream>
#include <boost/lexical_cast.hpp>

#include "TimeSlot.hpp"
#include "Str.hpp"

using namespace boost::posix_time;

namespace ecf {

///////////////////////////////////////////////////////////////////////////////////////////

bool TimeSlot::operator<(const TimeSlot& rhs) const
{
   if (h_ <  rhs.hour()) return true;
   if (h_ == rhs.hour()) {
      return m_ < rhs.minute();
   }
   return false;
}

bool TimeSlot::operator>(const TimeSlot& rhs) const
{
   if (h_ >  rhs.hour()) return true;
   if (h_ == rhs.hour()) {
      return m_ > rhs.minute();
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
   if (isNULL())  return "00:00";

   std::string ret;
	if (h_ < 10)  ret += "0";
	ret += boost::lexical_cast<std::string>(h_);

	ret += Str::COLON();
	if (m_ < 10) ret += "0";
	ret += boost::lexical_cast<std::string>(m_);
	return ret;
}

boost::posix_time::time_duration TimeSlot::duration() const
{
	assert(!isNULL());
	return boost::posix_time::time_duration( hours(h_) + minutes(m_) ) ;
}

std::ostream& operator<<(std::ostream& os, const TimeSlot* d) {
	if (d) return d->print(os);
	return os << "TimeSlot == NULL";
}
std::ostream& operator<<(std::ostream& os, const TimeSlot& d)  { return d.print(os); }

}
