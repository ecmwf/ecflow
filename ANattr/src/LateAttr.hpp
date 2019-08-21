#ifndef LATEATTR_HPP_
#define LATEATTR_HPP_
//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #13 $ 
//
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description :
//============================================================================

#include <boost/date_time/posix_time/posix_time_types.hpp>
#include "TimeSlot.hpp"
#include "NState.hpp"

namespace ecf { class Calendar;} // forward declare class

namespace ecf {
/// ========================================================================
/// Use compiler,  destructor, assignment, copy constructor,
///
/// ************************************************************************
/// late attribute: Only applies to a task, it can be set on suite/family
///                 but this is treated as an inherited attribute.
///                 late attribute lower down the hierarchy overrides it.
/// ************************************************************************
/// The late late attribute will not work correctly when the suites clock
/// start and stops with the server. Since the late relies on real time
/// for some of its functionality.
/// -s submitted: The time node can stay submitted (format [+]hh:mm). submitted is always
///               relative, so + is simple ignored, if present. If the node stays submitted
///               longer than the time specified, the late flag is set
/// -a Active   : The time of day the node must have become active (format hh:mm). If the node
///               is still queued or submitted, the late flag is set
/// -c Complete : The time node must become complete (format {+}hh:mm). If relative, time is
///               taken from the time the node became active, otherwise node must be complete by
///               the time given.
/// ========================================================================
class LateAttr  {
public:
   LateAttr();

	void print(std::string&) const;
	bool operator==(const LateAttr& rhs) const;

	void addSubmitted( const TimeSlot& s)    { s_  = s; }
	void add_submitted(int hour, int minute) { s_  = TimeSlot(hour,minute); }
	void addActive( const TimeSlot& s )      { a_  = s; }
	void add_active(int hour, int minute)    { a_  = TimeSlot(hour,minute); }
	void addComplete( const TimeSlot& s,  bool relative)   { c_  = s;                     c_is_rel_ = relative;}
	void add_complete(int hour, int minute,bool relative)  { c_  = TimeSlot(hour,minute); c_is_rel_ = relative; }

	const TimeSlot& submitted() const { return s_; }
	const TimeSlot& active() const { return a_; }
	const TimeSlot& complete() const { return c_; }
	bool complete_is_relative() const { return c_is_rel_; }

	/// i.e no time structs specified
	bool isNull() const;

	/// Given the state and time of state change, and calendar work out if we are late
	/// if we are sets the late flag
   void checkForLateness( const std::pair<NState,boost::posix_time::time_duration>&  state, const ecf::Calendar& c );
   bool check_for_lateness( const std::pair<NState,boost::posix_time::time_duration>&  state, const ecf::Calendar& c ) const;

	///  To be used by GUI to inform used that a node is late
	bool isLate() const { return isLate_;}

	/// To be called at begin and re-queue time
	void reset() { setLate(false); }

	// Overide this late attributes with the settings form the input.
	void override_with(LateAttr*);

	// The state_change_no is never reset. Must be incremented if it can affect equality
 	unsigned int state_change_no() const { return state_change_no_; }

 	/// set flag to be late
	void setLate(bool f);

	std::string toString() const;
   std::string name() const { return toString(); }
  
   static void parse(LateAttr&, const std::string& line, const std::vector<std::string >& lineTokens, size_t index);
   static LateAttr create(const std::string& lateString);

private:
   void write(std::string&) const;

private:
	TimeSlot s_;                    // relative by default
	TimeSlot a_;
	TimeSlot c_;
	unsigned int state_change_no_{0};  // *not* persisted, only used on server side
	bool c_is_rel_{false};
	bool isLate_{false};

private:
   friend class cereal::access;
   template<class Archive>
   void serialize(Archive & ar);
};

}
#endif
