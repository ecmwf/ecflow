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

#include "boost/date_time/posix_time/posix_time_types.hpp"
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/level.hpp>
#include <boost/serialization/tracking.hpp>
#include <ostream>

namespace ecf {

// Use compiler , generated destructor, assignment,  copy constructor
// *relative* times can extend to a maximum of 99 hours and 59 seconds
//
// TimeSlot is used in many other attributes, i.e. like AutoCancelAttr,AutoArchiveAttr
// in this case user can specify days, which we convert to hours, hence it
// is possible for a TimeSlot hour to have any integer value
class TimeSlot {
public:
	TimeSlot()
		: hour_(0), minute_(0),isNull_(true) {}
	TimeSlot(int hour, int min)
		: hour_(hour), minute_(min),isNull_(false)
		{ assert(hour >= 0  && min >=0 ); }
	explicit TimeSlot(const boost::posix_time::time_duration& td)
		: hour_(td.hours()), minute_(td.minutes()),isNull_(false)
		{ assert( hour_ < 60 && minute_ < 60);}

	bool operator==(const TimeSlot& rhs) const
		{ return ((hour_ == rhs.hour_) && (minute_ == rhs.minute_) && (isNull_ == rhs.isNull_));}
	bool operator!=(const TimeSlot& rhs) const
		{ return !operator==(rhs);}

   bool operator<(const TimeSlot& rhs) const;
   bool operator>(const TimeSlot& rhs) const;
   bool operator<=(const TimeSlot& rhs) const;
   bool operator>=(const TimeSlot& rhs) const;

	int hour() const { return hour_;}
	int minute() const { return minute_;}
	bool isNULL() const { return isNull_; }
	std::ostream& print(std::ostream&) const;

	/// returns the corresponding duration.
 	boost::posix_time::time_duration duration() const;

 	// returns  struct in the format hh:mm
 	std::string toString() const;

private:
 	unsigned short hour_;
 	unsigned short minute_;
 	bool           isNull_;

 	friend class boost::serialization::access;
 	template<class Archive>
 	void serialize(Archive & ar, const unsigned int /*version*/) {
 	   ar & hour_;
 	   ar & minute_;
 	   ar & isNull_;
 	}
};

std::ostream& operator<<(std::ostream& os, const TimeSlot*);
std::ostream& operator<<(std::ostream& os, const TimeSlot&);
}

// This should ONLY be added to objects that are *NOT* serialised through a pointer
BOOST_CLASS_IMPLEMENTATION(ecf::TimeSlot, boost::serialization::object_serializable);
BOOST_CLASS_TRACKING(ecf::TimeSlot,boost::serialization::track_never);

#endif
