#ifndef CALENDAR_HPP_
#define CALENDAR_HPP_
//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #48 $ 
//
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description : The calendar object is initialised when the suite begins.
//               The calendar encapsulates date and time. The date is derived from the time.
//
// After each update the calendar will store the time_duration between the init() function
// call and the update() function call.
// The calendar is to be used for all reference to time and date
// This will stop different time functionality from getting out of step.
//
// Examples of Use of calendar are:
//    o Generated time variables
//    o Time dependent attributes: TimeAttr,TodaySeries,CronAttr,DateAttr,DayAttr
//      ****************************************************************************
//      ** A time attribute can have a +, which means time relative to suite start
//      ** This is stored on the TimeSeries, as it isNode/Attribute specific.
//      ** i.e repeated families will have its own relative start time.
//      *****************************************************************************
//
// DESIGN CONSIDERATIONS:
//   Real and Hybrid:
//   Real
//      calendar is like a normal calendar where time and date are related
//      and day/date changes at midnight.
//   Hybrid:
//      There is currently confusion about how this is supposed to work.
//      The date is not supposed to change. (According to John date updates at suite restart?)
//      This has important implications, i.e does the day change ?
//      If the day does not change, then many of suites will never complete.
//      	i.e if we use repeat, with a single time series,  "time 10:00"
//      *** This relies on a day change to reset time attribute at midnight. ****
//   Conclusion: Will support day change for both REAL and HYBRID (date does not change)
//
// Calendar Updates:
// 	 How and when should we update the calendar?
//   In both the approaches below we need to make a distinction/separation
//   between the server poll and calendar update. This is required for testing
//
//	o Poll/Job submission interval in server is used to update calendar .
//     +: No time slots will be missed. even if server is suspended/restarted.
//        since suspending the server, also suspends the calendar updates
//     +: Suite relative times will continue to work even after server stopped/started
//     +: Avoids additional system call.
//     +: Lead's to more deterministic behaviour
//     -: If server is suspended and restarted the calendar will NOT be in
//        phase with system clock. (Its not clear to me why this should be an issue?)
//     ?: If the server is run for several days is there a possibility for the poll
//        update to get out of skew with real time. This is only possible if
//        job dependencies take more the 60 seconds to resolve.
//      *** THIS FUNCTIONALITY NEEDS TO BE ADVERTISED, SO THAT USERS ARE AWARE OF IT
//      *** THIS FUNCTIONALITY IS AVAILABLE VIA -s flag on the clock attribute .i.e
//      *** the -s stand's for stop start clock in line with the server
// 	        clock real 20.1.2007 +01:00 -s
// 			clock hybrid -s
//
//	o Poll/Job submission interval in server is used Initiate an update of calendar via a system call.
//     +: calendar is always in phase with system clock.
//        Many task job dependencies depend on ordering based on real time.
//     -: Requires additional system call for each poll in the server
//     -: Time slots can be missed. (i.e if server suspended/restarted).
//        There is no catch up. (?? See TimeDependencies.ddoc)
//     -: Relative times will not be adhered too, when server stopped/started.
//     -: Will require more manual intervention ?
//
//  Conclusion:: The clock attribute will be changed to add both capabilities
//
//  Resolution: Will support 1 minute resolution:
//============================================================================

#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/date_time/posix_time/conversion.hpp>
#include <boost/date_time/posix_time/time_serialize.hpp>
#include "Serialization.hpp"
#include "cereal.hpp"

namespace ecf {

class CalendarUpdateParams; // forward declare

class Calendar  {
public:
	enum Clock_t {
		REAL,       // like a normal clock
		HYBRID      // date does not change, but will support day change. See Above.
	};

	/// Defaults to the REAL clock
	Calendar();
	Calendar(const Calendar&);
	Calendar& operator=( const Calendar&);
	bool operator==( const Calendar&) const;

	/// Initialise the Calendar.
	/// The boolean startStopWithServer allows us to choose how we update the calendar:
	//      False: Use system time to update the calendar:
	//      True : Use the server poll, to update the calendar
  	void init(Clock_t clock, bool startStopWithServer = false);

   // for test init and begin calendar
   void init(const boost::posix_time::ptime& time, Clock_t clock = Calendar::REAL, bool startStopWithServer = false);

   /// Start the Calendar.  Parameter time can include gain.
   void begin(const boost::posix_time::ptime&);

	/// Update the calendar using the input. Will Store internally the time duration
	/// between the init() function call and the last update.
 	/// The for_test parameter is *ONLY* used if we are using real time calendar
  	void update(const ecf::CalendarUpdateParams &);

  	// Used for test only, will call the function above
  	void update(const boost::posix_time::time_duration&);
  	void update(const boost::posix_time::ptime& time_now);

	// The following were added as a performance optimisation
   // Represent a day within a week (range 0==Sun to 6==Sat)
	int day_of_week() const;  // same as suiteTime().date().day_of_week().as_number()
	int day_of_year() const;  // same as suiteTime().date().day_of_year()
	int day_of_month() const; // same as suiteTime().date().day()
	int month() const;        // same as suiteTime().date().month()
	int year() const;         // same as suiteTime().date().year()

	/// returns true if the day changed, this will update for REAL and HYBRID .
  	bool dayChanged() const { return dayChanged_;}

  	/// returns the last calendar increment
	const boost::posix_time::time_duration& calendarIncrement() const { return calendarIncrement_;}

 	/// return the init() time + the accumulated duration from calls to update(...)
  	/// This should only be used when this calendar is real.
 	const boost::posix_time::ptime& suiteTime() const { return suiteTime_;}

 	// duration since last call to init, essentially suite duration
	const boost::posix_time::time_duration& duration() const { return duration_;}

	/// return real time, when the calendar was begun/initialised.
	/// This is used to update the duration_, which is recorded for each state change in the node
	/// Hence to when we can compute when a state change occurred by using:
	///   boost::posix_time::ptime time_of_state_change = begin_time() + node->get_state().second(duration)
   const boost::posix_time::ptime& begin_time() const { return initLocalTime_;}

 	/// return the date, for real calendar this corresponds to the date on suiteTime_
 	/// for hybrid,  the date does not change, and hence we return date for initTime_
	boost::gregorian::date date() const;

 	/// The calendar type. For hybrid clocks the date does not update.
	bool hybrid() const { return (ctype_ == Calendar::HYBRID) ?  true : false; }

	/// for debug,  must link with boost date and time library
	void dump(const std::string& title) const;

	/// for debug,  must link with boost date and time library
	std::string toString() const;

	std::string write_state() const;
   void read_state(const std::string& line,const std::vector<std::string>& lineTokens);

	bool checkInvariants(std::string& errorMsg) const;

	// allow Suite memento to update calendar type
	void set_clock_type( Clock_t  ct) { ctype_ = ct;}

	/// Will return either second_clock::universal_time()/UTC ( the other alternative is second_clock::local_time() )
	/// This is because boost deadline timer is based on UTC clock
	/// >>> The deadline_timer typedef is based on a UTC clock, and all operations (expires_at, expires_from_now) work in UTC time.
	/// >>> If you want it to use a different clock (such as the local time clock), you can use basic_deadline_timer<> with your own traits class.
	/// >>> Please see the "Timers" example.
	/// Taken from boost date/time doc
  	/// If you want exact agreement with wall-clock time, you must use either UTC or local time. If you compute a duration
	//	by subtracting one UTC time from another and you want an answer accurate to the second, the two times must not be
	//	too far in the future because leap seconds affect the count but are only determined about 6 months in advance. With
	//	local times a future duration calculation could be off by an entire hour, since legislatures can and do change DST
	//	rules at will.
	//	If you want to handle wall-clock times in the future, you won't be able (in the general case) to calculate exact durations,
	//	for the same reasons described above.
	//	If you want accurate calculations with future times, you will have to use TAI or an equivalent, but the mapping from
	//	TAI to UTC or local time depends on leap seconds, so you will not have exact agreement with wall-clock time.
	static boost::posix_time::ptime second_clock_time();


private:
	void assign( const Calendar& rhs);

	Clock_t                          ctype_;      // *NOT* persisted: can be derived from suite clock attribute
 	boost::posix_time::ptime         initTime_;   // When calendar was started, suite time(could be in the past OR real time)
 	boost::posix_time::ptime         suiteTime_;  // The suite time for hybrid DATE does not change.
	boost::posix_time::time_duration duration_;   // duration since last call to init/begin, used on Node for late and autocancel
 	bool                             dayChanged_;
 	bool                             startStopWithServer_; //*NOT* persisted: false means real time calendar,  can be derived from suite clock attribute

  	boost::posix_time::ptime         initLocalTime_;   // Real Time: When calendar was started, used to work out duration_
 	boost::posix_time::ptime         lastTime_;        // Real Time: Used to calculate calendarIncrement

	boost::posix_time::time_duration calendarIncrement_;

private:
   void update_cache() const;
	mutable int day_of_week_;  // Cache
	mutable int day_of_year_;  // Cache
	mutable int day_of_month_; // Cache
	mutable int month_;        // Cache
	mutable int year_;         // Cache

private:
	// Note: The *only* reason to serialise the calendar is so that we can support
	// why() command on the client side. By default calendar is initialised in the *server*
	// at begin time, from the clock attribute
   friend class cereal::access;
	template<class Archive>
	void serialize(Archive & ar, std::uint32_t const /*version*/)
	{
	   if (Archive::is_saving::value) {
	      if ( initTime_.is_special() ) {
	         // Initialise the ptimes to avoid serialisation exceptions
	         // The serialisation of ptime makes use of exceptions, especially
	         // when dealing with a date that has *not* been initialised.
	         // To avoid this we take a small hit to initialise the calendar with
	         // time now. This will get overriden with suite clock at begin
	         begin(second_clock_time());
	      }
	   }
      ar( CEREAL_NVP(initTime_),
          CEREAL_NVP(suiteTime_),
          CEREAL_NVP(duration_),
          CEREAL_NVP(dayChanged_),
          CEREAL_NVP(initLocalTime_),
          CEREAL_NVP(lastTime_),
          CEREAL_NVP(calendarIncrement_)
        );
	}
};
}

#endif
