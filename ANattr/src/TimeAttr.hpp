#ifndef TIMEATTR_HPP_
#define TIMEATTR_HPP_
//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #32 $ 
//
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description :
//
/// isFree is called when a node is queued.If it returns true, Task can be submitted
/// checkForReque: is called when a node has completed, and need to determine if it should run again.
/// These are different/orthogonal concerns.
/// There is a *separate* issue of whether nodes should be queued when a node is *manually*
///    a/ Set complete
///    b/ Runs and then completes
///
/// For a *single* time slot we can't requeue.
/// Hence we checkForReque that takes as parameters max/min time slots, so we **treat**
/// Multiple single slot as a series.
///
///                                               isFree:hhhhhhhhhhhhhhhhh
///                                               Begin:
///                                               V
///checkForReque:rrrrrrrrrrrrrrrrhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhh   10:00
///checkForReque:rrrrrrrrrrrrrrrrrrrrrrrrrrrrrhhhhhhhhhhhhhhhhhhhhhhh   11:00
///checkForReque:rrrrrrrrrrrrrrrrrrrrrrrrrrrrrhhhhhhhhhhhhhhhhhhhhhhh   for both 10:00 and 11:000 together
///       isFree:hhhhhhhhhhhhhhhhffffffffffffffffffffffffffffffffffff   *once* free we stay free (single slot *only*)
///       begin :                |                           |
///        V                     |                           |
/// Time   ======================0============0==============0=============
///                            10:00        11:00              Midnight
//
///                                                     isFree:hhhhhhhhhhhhhhhhhh
///                                                      V
///   CheckForReque:rrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrhhhhhhhhhhhhhhhhhhhhhhhhhhhhh
///          isFree:hhhhhhhFhhhhFhhhhFhhhhFhhhhFhhhhFhhhhhhhhhhhhhhhhhhhhhhhhhhhh
///            V           |    |    |    |    |    |        |
/// Time   ================o====|====|====|====|====0========0====================
///                      10:00  1    2    3    4  15:00    Midnight
///
/// If the job starts at 10:00 but takes more than 1 hour, then it will miss the 11:00 slot
/// and will have to start at 12:00
//============================================================================
#include <boost/serialization/tracking.hpp>
#include <boost/serialization/level.hpp>
#include "TimeSeries.hpp"

namespace ecf { class Calendar;} // forward declare class

namespace ecf {

// Use compiler ,  destructor, assignment, copy constructor,
class TimeAttr  {
public:
	TimeAttr() : makeFree_(false), state_change_no_(0) {}
	TimeAttr(int hour, int minute, bool relative = false )
		: timeSeries_(hour, minute,relative), makeFree_(false),state_change_no_(0) {}
	TimeAttr(const TimeSlot& t,    bool relative = false )
		: timeSeries_(t,relative), makeFree_(false),state_change_no_(0) {}
	TimeAttr(const TimeSeries& ts)
		: timeSeries_(ts), makeFree_(false),state_change_no_(0) {}
	TimeAttr(const TimeSlot& start, const TimeSlot& finish, const TimeSlot& incr, bool relative = false)
		: timeSeries_(start,finish,incr,relative), makeFree_(false),state_change_no_(0) {}

	std::ostream& print(std::ostream&) const;
   bool operator==(const TimeAttr& rhs) const;
   bool operator<(const TimeAttr& rhs) const { return timeSeries_ < rhs.timeSeries_; }
	bool structureEquals(const TimeAttr& rhs) const;

	/// This can set attribute as free, once free its stays free, until re-queue/reset
	void calendarChanged( const ecf::Calendar& c ); // can set attribute free
	void resetRelativeDuration();

	void reset(const ecf::Calendar& c)
	      { clearFree(); timeSeries_.reset(c); }       // updates state_change_no_
 	void requeue(const ecf::Calendar& c,bool reset_next_time_slot = true)
 	      { clearFree(); timeSeries_.requeue(c,reset_next_time_slot);} // updates state_change_no_

	void miss_next_time_slot(); // updates state_change_no_
	void setFree();   // ensures that isFree() always returns true, updates state_change_no_
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
	void clearFree(); // resets the free flag, updates state_change_no_
   bool is_free(const ecf::Calendar&) const; // ignores makeFree_

private:
 	TimeSeries   timeSeries_;
	bool         makeFree_;
	unsigned int state_change_no_;  // *not* persisted, only used on server side

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int /*version*/)
    {
     	ar & timeSeries_;
     	ar & makeFree_;      // Only persisted for testing, see usage of isSetFree()
    }
};

}

// This should ONLY be added to objects that are *NOT* serialised through a pointer
BOOST_CLASS_IMPLEMENTATION(ecf::TimeAttr, boost::serialization::object_serializable);
BOOST_CLASS_TRACKING(ecf::TimeAttr,boost::serialization::track_never);

#endif
