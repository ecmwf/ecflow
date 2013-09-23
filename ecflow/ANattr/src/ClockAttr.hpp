#ifndef CLOCKATTR_HPP_
#define CLOCKATTR_HPP_

//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #18 $ 
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

#include <ostream>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/serialization/serialization.hpp>

namespace ecf { class Calendar;} // forward declare class that is in a namesapce

/// The clock attribute is defined on the suite ONLY
/// Use default copy constructor and assignment operator, destructor
/// The clock attribute is used to initialise the calendar object
/// **********************************************************************
/// In the OLD sms the date is actually used as a gain factor(well at least
/// according to the user/reference manual), in the ec-flow
/// a date, is a date. i.e. it allows us to start a suite in the past.
///
/// The Constructor will update the State change number, since it can be added
/// by the AlterCmd. in the Client Context, state change number is not incremented
/// ************************************************************************
///
class ClockAttr {
public:
	/// The following constructor is used for test only. It allows us to
	/// create a clock attribute initialised with given date and time
	ClockAttr(const boost::posix_time::ptime&, bool hybrid = true, bool positiveGain = true);
	ClockAttr(int day, int month, int year, bool hybrid = true );
	ClockAttr(bool hybrid = true);

	std::ostream& print(std::ostream&) const;
	bool operator==(const ClockAttr& rhs) const;

	void date(int day, int month, int year);
	void set_gain(int hour,int min,bool positiveGain = true);
	void set_gain_in_seconds(long theGain,bool positiveGain = true);

	void startStopWithServer(bool f);
	bool startStopWithServer() const { return startStopWithServer_; }
 	void hybrid( bool f );

   void init_calendar(ecf::Calendar&);
   void begin_calendar(ecf::Calendar&);

	// The state_change_no is never reset. Must be incremented if it can affect equality
 	unsigned int state_change_no() const { return state_change_no_; }

	// access
	int day() const { return day_; }
	int month() const { return month_; }
	int year() const { return year_; }
 	long gain() const { return gain_;}
 	bool positive_gain() const { return positiveGain_;}
 	bool hybrid() const { return hybrid_;}
 	bool is_virtual() const { return startStopWithServer_;}
	std::string toString() const;

private:
	bool hybrid_;
	bool positiveGain_;
	bool startStopWithServer_;  // see Calendar.hpp for more details
	long gain_;                 // in seconds
	int day_;
	int month_;
	int year_;

	unsigned int   state_change_no_;  // *not* persisted, only used on server side

	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int /*version*/)
	{
	   ar & hybrid_;
	   ar & positiveGain_;
	   ar & startStopWithServer_;
	   ar & gain_;
	   ar & day_;
	   ar & month_;
	   ar & year_;
	}
};

#endif
