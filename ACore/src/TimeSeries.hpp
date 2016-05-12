#ifndef TIMESERIES_HPP_
#define TIMESERIES_HPP_
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #57 $ 
//
// Copyright 2009-2016 ECMWF. 
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description : A single or set of times
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8

#include "TimeSlot.hpp"
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/date_time/posix_time/time_serialize.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/level.hpp>
#include <boost/serialization/tracking.hpp>

namespace ecf { class Calendar;} // forward declare class

namespace ecf {
/// TimeSeries can have a single time slot or a series of time slots
///
///
/// We need nextTimeSlot_ data member to record the next valid time slot, otherwise when
/// we have a time series, all times within the time series would be valid for
/// Job submission. We do not know how long a job will run, hence when incrementing
/// the nextTimeSlot_, we need the *NEXT* available time slot *after* the current calendar time.
///
/// Usage pattern is :
///           TimeSeries.calendarChanged()   // Called every minute. Calls isFree(). Once free we stay free, until requeue.
///                                          // At midnight clear time expiration and reset next time slot
///           TimeSeries.isFree(..)          // called when node is QUEUED:
///                                          // during dependency evaluation, checks next time slot, against calendar time
///                                          // Once free a node stays free, until requeue()
///           TimeSeries.checkForRequeue();  // called when node is COMPLETE: Checks if next available time slot is valid
///                                          // *Must* return false for last time slot
///                                          // ECFLOW-130 jobs that start before midnight and finish after midnight should not requeue
///           TimeSeries.requeue();          // called when node is re-queued.
///                                          // Sets value to next valid time slot after calendar time.
///                                          // Can *expire* the time, relies on TimeSeries.calendarChanged() to clear time expiration
///                                          // after midnight
/// ECFLOW-130 jobs that start before midnight and finish after midnight should not requeue

// Use compiler, destructor, copy constructor and assignment operator
class TimeSeries  {
public:
	TimeSeries();
	TimeSeries(int hour, int minute, bool relativeToSuiteStart = false );
	TimeSeries(const TimeSlot&, bool relativeToSuiteStart = false );
	TimeSeries(const TimeSlot& start, const TimeSlot& finish, const TimeSlot& incr,bool relativeToSuiteStart = false);

   bool operator<(const TimeSeries& rhs) const;

	// returns true if a state change is made
   // Will clear time expiration flag at midnight
   bool calendarChanged( const ecf::Calendar& c);

	// relative duration stored locally since it can be reset, when used with repeats
	// returns true if relative duration is reset/ i.e state change has been made
	bool resetRelativeDuration();
	void reset(const ecf::Calendar& c);

	// Increment time series. Will find the next time slot after current calendar
	// record the current suite time, to check of jobs finish after midnight
	void requeue(const ecf::Calendar& c,bool reset_next_time_slot = true);

	// Since we can miss next time slot , allow its computation for the
	// use with why command, returns a NULL timeslot if next time is invalid
	// This functionality will mirror the requeue(..) function
   TimeSlot compute_next_time_slot(const ecf::Calendar& c) const;

   // Return true calendar is before or within scheduled time
   bool requeueable(const ecf::Calendar& c) const;

	/// if relativeToSuiteStart returns the relative duration, else returns calendar suite time of day.
	/// The returned resolution is in minutes
 	boost::posix_time::time_duration duration(const ecf::Calendar& calendar ) const;

 	/// If time has expired, returns false
	/// Note: relative means relative to suite start, or relative to the beginning of repeated node.
 	/// Is of the form hh:mm.   This means relative has a range 00:00->99.59
	/// For a single slot time series, should not be re queued again, and hence
	/// should fail checkForRequeue. Likewise when the the current value is
	/// the finish, then node should also not be re queued, otherwise the
	/// node will be stuck in queued state.
	bool isFree(const ecf::Calendar& calendar) const;

	/// For single slot time based attributes we need additional context (i.e the_min,the_max parameter)
	/// in order to determine whether we should re-queue.
	/// Additionally when we have a time range, what if the last jobs runs over midnight. In this case
	/// we need to return false. i.e do not re-queue ECFLOW-130
   bool checkForRequeue( const ecf::Calendar& calendar, const TimeSlot& the_min, const TimeSlot& the_max) const;

   void min_max_time_slots(TimeSlot& the_min, TimeSlot& the_max) const;

	// Called when explicitly Free holding time dependency. via FreeDepCmd
	// We want to avoid the next time slot.
   void miss_next_time_slot();

 	void why(const ecf::Calendar&, std::string& theReasonWhy) const;

 	bool hasIncrement() const { return !finish_.isNULL();}
	const TimeSlot& start()  const  { return start_; }
	const TimeSlot& finish() const  { return finish_;}
	const TimeSlot& incr()   const  { return incr_;  }
	const TimeSlot& value()  const  { return nextTimeSlot_; }
	bool relative()  const  { return relativeToSuiteStart_; }
	void free_slots(std::vector<boost::posix_time::time_duration>& ) const;

	std::ostream& print(std::ostream&) const;
 	bool operator==(const TimeSeries& rhs) const;
	bool operator!=(const TimeSeries& rhs) const { return !operator==(rhs);}
   bool structureEquals(const TimeSeries& rhs) const;

	/// returns true if no time specified
	bool isNULL() const { return  start_.isNULL(); }

   std::string state_to_string(bool isFree) const;
   std::string toString() const;
  	std::string dump() const;
 	bool checkInvariants(std::string& errormsg) const;

	/// expects HH:MM or +HH:MM will throw std:runtime_error for errors,
 	/// *if* hour not in range(0-24), minutes(0-59), *and* check_time parameter is enabled
	static bool getTime(const std::string& time, int& hour, int& min, bool check_time = true);

	/// extract string like
	///     time +00:00 20:00 00:10 # this is a comment which will be ignored. index = 1
	///     time +20:00        // index = 1
	///     today 20:00        // index = 1
	///     +00:00 20:00 00:10 // index = 0
	///     +20:00             // index = 0
	/// will throw std:runtime_error for errors
	/// will assert if index >= lineTokens.size()
	static ecf::TimeSeries create(size_t& index, // where we should start
	                              const std::vector<std::string>& lineTokens,
	                              bool read_state = false);

	// like above but string should not contain "time"
	static ecf::TimeSeries create(const std::string& str);

	// Parse state associated with the time series: Ignores attributes like free.
	static void parse_state(size_t index,const std::vector<std::string>& lineTokens, ecf::TimeSeries& ts);
	void set_isValid(bool b) { isValid_= b;} // for test only
	void set_next_time_slot( const TimeSlot& ts) { nextTimeSlot_ = ts; } // needed for test only
	const TimeSlot& get_next_time_slot() const { return nextTimeSlot_;}

	// Is the time still valid, return false means time has expired.
	bool is_valid() const { return isValid_;}
private:

	static void testTime(int hour, int minute);

	// HANDLE CASE WHERE FINISH MINUTES IS NOT DIVISIBLE BY THE INCREMENT
	// time 00:00 23:59 00:10  last valid time is 23:50
	// time 00:30 23:59 04:00  last valid time is 20:30
	// The last valid time here is 23:50/20:30 must return false, to stop node being
	// stuck in queued mode In the case where the FINISH is NOT a multiple of the increment
	// Need a different mechanism to determine the end.
	void compute_last_time_slot(); // only call for series

	bool match_duration_with_time_series(const boost::posix_time::time_duration& relative_or_real_td) const;
	boost::posix_time::time_duration relativeDuration() const;

private:
	bool relativeToSuiteStart_;
	bool isValid_;                                     // Needed for single slot to avoid multiple jobs submissions
	TimeSlot start_;
	TimeSlot finish_;
	TimeSlot incr_;
	TimeSlot nextTimeSlot_;                             // nextTimeSlot_ >= start && is incremented by incr
	mutable TimeSlot suiteTimeAtReque_; // NOT persisted, check of day change, between requeue -> checkForRequeue, when we have series
	boost::posix_time::time_duration relativeDuration_;
	boost::posix_time::time_duration lastTimeSlot_;    // Only used when we have a series, can be generated

private:
	// Note: isValid_      is persisted for use by why() command on the client side.
	// Note: nextTimeSlot_ is persisted for use by why() command on the client side.
	// Note: relativeDuration_ is persisted for use by why() command on the client side.
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int /*version*/)
	{
	   ar & relativeToSuiteStart_;
	   ar & isValid_;
	   ar & start_;
	   ar & finish_;
	   ar & incr_;
	   ar & nextTimeSlot_;
	   ar & relativeDuration_;

	   if (Archive::is_loading::value) {
	      if (!finish_.isNULL()) {
	         compute_last_time_slot();
	      }
	   }
	}
};

std::ostream& operator<<(std::ostream& os, const TimeSeries*);
std::ostream& operator<<(std::ostream& os, const TimeSeries&);
}

// This should ONLY be added to objects that are *NOT* serialised through a pointer
BOOST_CLASS_IMPLEMENTATION(ecf::TimeSeries, boost::serialization::object_serializable);
BOOST_CLASS_TRACKING(ecf::TimeSeries,boost::serialization::track_never);

#endif
