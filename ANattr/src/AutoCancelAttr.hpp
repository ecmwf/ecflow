#ifndef AUTOCANCELATTR_HPP_
#define AUTOCANCELATTR_HPP_
//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #9 $ 
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

#include "TimeSlot.hpp"

namespace ecf { class Calendar;} // forward declare class

namespace ecf {

// Use compiler ,  destructor, assignment, copy constructor
class AutoCancelAttr  {
public:
	AutoCancelAttr() : relative_(true),days_(false) {}
	AutoCancelAttr(int hour, int minute, bool relative ) : timeStruct_(hour, minute), relative_(relative), days_(false) {}
 	AutoCancelAttr(const TimeSlot& ts,   bool relative ) : timeStruct_(ts),           relative_(relative), days_(false) {}
 	explicit AutoCancelAttr(int days) : timeStruct_( TimeSlot(days*24,0) ), relative_(true), days_(true) {}

	std::ostream& print(std::ostream&) const;
	bool operator==(const AutoCancelAttr& rhs) const;
	bool isFree(const ecf::Calendar&, const boost::posix_time::time_duration& suiteDurationAtComplete) const;

	std::string toString() const;

	const TimeSlot& time() const { return timeStruct_;}
	bool relative() const { return relative_; }
	bool days() const { return days_; }

private:
 	TimeSlot timeStruct_;
 	bool relative_;
 	bool days_;

 	friend class cereal::access;
 	template<class Archive>
 	void serialize(Archive & ar, std::uint32_t const /*version*/)
 	{
 	   ar( CEREAL_NVP(timeStruct_ ),
 	       CEREAL_NVP(relative_ ),
 	       CEREAL_NVP(days_ )
 	   );
 	}
};

}
#endif

