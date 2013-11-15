#ifndef TODAYATTR_HPP_
#define TODAYATTR_HPP_
//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #30 $ 
//
// Copyright 2009-2012 ECMWF. 
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description : The Today attribute is heavily tied to the begin command
//               Real Clock:
//               1/ Suite Begin time > Today time
//                  If the suite 'begin' time is past the time given for today
//                  the node is free to run.
//               2/ Suite Begin time < Today Time
//                  The node will 'hold' until current time > today time
//               3/ Suite time, has passed midnight(next day)
//                  then today command will permanently hold the node
//               4/ Under Real/hybrid clocks today will hold node after
//                  current is past last today time.
//
// take following example when we have a single time slot:
//  	today 10:00
//                                                isFree:-----free-----
//                                                 begin:
//                                                 V
// checkForReque:rrrrrrrrrrrrrrrrhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhh
//         isFree:hhhhhhhhhhhhhhhh|fffffffffffffffffffffffffffffffffffffff  *once* free we stay free (single slot *only*)
//         begin:
//          V
//   Today  ======================0=====================0=================
//                               10:00              Midnight
//
//   Difference between time and today. If begin is started after the time slot
//   then the node is free to re-run
//
// When we have a today time series:
//  	today 10:00 20:00 01:00
//
//  *** If the begin time is past 10:00  in the case above then the
//  *** node should is free to run once. However for a range its different
//  *** if suite begin time is past 20:00 then the node is held.
//.
//  At 10am the Node is free, when node completes, it is re-queued
//  At 11am the Node is free, when node completes, it is re-queued
//  ....
//  At 20pm the Node is free, when node completes, it is *NOT* re-queued.
//--------------------------------------------------------------------------------
/// isFree is called when a node is queued. if it returns true, Task can be submitted
/// checkForReque: is called when a node has completed, and need to determine if it should run again.
/// These are different/orthogonal concerns.
/// There is a *separate* issue of whether nodes should be queued when a node is *manually*
///    a/ Set complete
///    b/ Runs and then completes
///
/// For a *single* time slot we can't requeue.
/// ****However we could have a set of time slots *****
///
///                                               isFree:ffffffffffffffff
///                                               Begin:
///                                               V
///checkForReque:hhhhhhhhhhhhhhhhhrrrrrrrrhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhh
///       isFree:hhhhhhhhhhhhhhhh|fffffffffffffffffffffffffffffffffffffff
///       begin :
///        V
/// Today  ======================0========0===========0====================
///                            10:00    11:00      Midnight
///                                                     isFree:hhhhhhhhhhhhhhhhhh
///                                                      V
///   CheckForReque:hhhhhhhrrrrrrrrrrrrrrrrrrrrrrrrrhhhhhhhhhhhhhhhhhhhhhhhhhhhhh
///          isFree:hhhhhhhFhhhhFhhhhFhhhhFhhhhFhhhhFFFFFFFFFFFFFFFFFFFFFFFFFFFFF
///            V           |    |    |    |    |    |        |
/// Today  ================o====|====|====|====|====0========0====================
///                      10:00  1    2    3    4  15:00    Midnight
///
/// If the job starts at 10:00 but takes more than 1 hour, then it will miss the 11:00 slot
/// and will have to start at 12:00
//============================================================================

#include <boost/serialization/level.hpp>
#include <boost/serialization/tracking.hpp>

#include "TimeSeries.hpp"


namespace ecf { class Calendar;} // forward declare class

namespace ecf {

// Use compiler ,  destructor, assignment, copy constructor
class TodayAttr  {
public:
	TodayAttr() : makeFree_(false), state_change_no_(0) {}
	TodayAttr(int hour, int minute, bool relative = false )
		: timeSeries_(hour, minute,relative), makeFree_(false),state_change_no_(0) {}
 	TodayAttr(const TimeSlot& t,    bool relative = false )
		: timeSeries_(t,relative), makeFree_(false),state_change_no_(0) {}
	TodayAttr(const TimeSeries& ts)
		: timeSeries_(ts), makeFree_(false),state_change_no_(0) {}
	TodayAttr(const TimeSlot& start, const TimeSlot& finish, const TimeSlot& incr,bool relative =  false)
		: timeSeries_(start,finish,incr,relative), makeFree_(false),state_change_no_(0) {}

	std::ostream& print(std::ostream&) const;
	bool operator==(const TodayAttr& rhs) const;
   bool operator<(const TodayAttr& rhs) const { return timeSeries_ < rhs.timeSeries_; }
	bool structureEquals(const TodayAttr& rhs) const;

	void calendarChanged( const ecf::Calendar& c );  // can set attribute free
	void resetRelativeDuration();

	void reset(const ecf::Calendar& c) { clearFree(); timeSeries_.reset(c);}       // updates state_change_no_
	void requeue(const ecf::Calendar& c,bool reset_next_time_slot = true)
	   { clearFree(); timeSeries_.requeue(c,reset_next_time_slot);} // updates state_change_no_

	void miss_next_time_slot(); // updates state_change_no_
	void setFree();               // ensures that isFree() always returns true, updates state_change_no_
	void clearFree();             // resets the free flag, updates state_change_no_
	bool isSetFree() const { return makeFree_; }

   bool isFree(const ecf::Calendar&) const;
   bool checkForRequeue( const ecf::Calendar& c,const TimeSlot& the_min,const TimeSlot& the_max) const
	{ return timeSeries_.checkForRequeue(c,the_min,the_max);}
	void min_max_time_slots(TimeSlot& the_min, TimeSlot& the_max) const {timeSeries_.min_max_time_slots(the_min,the_max);}
 	bool why(const ecf::Calendar&, std::string& theReasonWhy) const;
 	bool checkInvariants(std::string& errormsg) const { return timeSeries_.checkInvariants(errormsg);}

	// The state_change_no is never reset. Must be incremented if it can affect equality
 	// Note: changes in state of timeSeries_, i.e affect the equality operator (used in test)
 	//       must be captured. i.e things like relative duration & next_time_slot are
 	//       reported by the Why command, & hence need to be synced.
 	unsigned int state_change_no() const { return state_change_no_; }

 	std::string name() const { return toString(); } /* ABO */
 	std::string toString() const;
 	std::string dump() const;

	// access
	const TimeSeries& time_series() const { return timeSeries_; }

private:
	bool is_free(const ecf::Calendar&) const; // ignores makeFree_

private:
	TimeSeries    timeSeries_;
	bool          makeFree_;         // persisted for use by why() on client side && for state changes
	unsigned int state_change_no_;  // *not* persisted, only used on server side

	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int /*version*/)
	{
	   ar & timeSeries_;
	   ar & makeFree_;
	}
};

}

// This should ONLY be added to objects that are *NOT* serialised through a pointer
BOOST_CLASS_IMPLEMENTATION(ecf::TodayAttr, boost::serialization::object_serializable);
BOOST_CLASS_TRACKING(ecf::TodayAttr,boost::serialization::track_never);

#endif
