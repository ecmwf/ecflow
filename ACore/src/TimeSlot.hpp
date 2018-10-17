#ifndef TIMESLOT_HPP_
#define TIMESLOT_HPP_
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

#include <iosfwd>
#include "boost/date_time/posix_time/posix_time_types.hpp"
#include <cereal/access.hpp>

namespace ecf {

// Use compiler , generated destructor, assignment,  copy constructor
// *relative* times can extend to a maximum of 99 hours and 59 seconds
//
// TimeSlot is used in many other attributes, i.e. like AutoCancelAttr,AutoArchiveAttr
// in this case user can specify days, which we convert to hours, hence it
// is possible for a TimeSlot hour to have any integer value
class TimeSlot {
public:
   static std::string type() { return "TimeSlot";}
	TimeSlot()= default;
	TimeSlot(int hour, int min) : h_(hour), m_(min) { assert(hour >= 0  && min >=0 ); }
	explicit TimeSlot(const boost::posix_time::time_duration& td)
		: h_(td.hours()), m_(td.minutes())
		{ assert( h_ < 60 && m_ < 60);}

	bool operator==(const TimeSlot& rhs) const { return ((h_ == rhs.h_) && (m_ == rhs.m_));}
	bool operator!=(const TimeSlot& rhs) const { return !operator==(rhs);}

   bool operator<(const TimeSlot& rhs) const;
   bool operator>(const TimeSlot& rhs) const;
   bool operator<=(const TimeSlot& rhs) const;
   bool operator>=(const TimeSlot& rhs) const;

	int hour() const { return h_;}
	int minute() const { return m_;}
	bool isNULL() const { return (h_ == -1 && m_ == -1); }
	std::ostream& print(std::ostream&) const;

	/// returns the corresponding duration.
 	boost::posix_time::time_duration duration() const;

 	// returns  struct in the format hh:mm
 	std::string toString() const;

private:
 	int h_{-1};
 	int m_{-1};

   // *IMPORTANT* no version for a simple class
 	friend class cereal::access;
 	template<class Archive>
 	void serialize(Archive & ar);
};

std::ostream& operator<<(std::ostream& os, const TimeSlot*);
std::ostream& operator<<(std::ostream& os, const TimeSlot&);
}

#endif
