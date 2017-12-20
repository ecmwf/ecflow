#ifndef DSTATE_HPP_
#define DSTATE_HPP_

//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #7 $ 
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

#include <string>
#include <vector>
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/level.hpp>
#include <boost/serialization/tracking.hpp>
#include "NState.hpp"

// DState: stores the state of a node.
// *The class DState just used to define the enum, however we also
// needed to know when the state changed. Hence the use of state_change_no
// Uses default copy constructor and destructor, and equality
class DState {
public:
   enum State { UNKNOWN =0, COMPLETE=1,  QUEUED=2, ABORTED=3, SUBMITTED=4, ACTIVE=5, SUSPENDED=6};
	DState(State s): state_(s), state_change_no_(0) {}
	DState(): state_(QUEUED),state_change_no_(0) {}

	State state() const { return state_;}
	void setState(State);

	// The state_change_no is never reset. Must be incremented if it can affect equality
 	unsigned int state_change_no() const { return state_change_no_; }

	bool operator==(const DState& rhs) const { return state_ == rhs.state_;}
	bool operator!=(const DState& rhs) const { return state_ != rhs.state_;}
	bool operator==(State s) const { return s == state_;}
	bool operator!=(State s) const { return s != state_;}

	static DState::State default_state() { return DState::QUEUED; }
	static NState::State convert(DState::State);
   static const char* toString(DState::State s);
   static const char* to_html(DState::State s);
   static const char* toString(const DState& ns) { return toString(ns.state());}
   static std::string to_string( DState::State s){ return std::string( toString(s) );}
	static DState::State toState(const std::string& state);
	static bool isValid(const std::string& state);
	static std::vector<std::string> allStates();
	static std::vector<DState::State> states();

private:
	State state_;
	unsigned int state_change_no_;  // *not* persisted, only used on server side

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int /*version*/)
    {
        ar & state_;
    }
};

////////////////////////////////////////////////////////////////////////////////////////
// Thin wrapper over DState, to aid python. i.e Task("t").add(Defstatus(DState.complete))
class Defstatus {
public:
   Defstatus(DState::State state) : state_(state) {}
   Defstatus(const std::string& ds) : state_(DState::toState(ds)) {}
   DState::State state() const { return state_;}
   std::string to_string() const { return DState::to_string(state_);}
private:
   DState::State state_;
};

// This should ONLY be added to objects that are *NOT* serialised through a pointer
BOOST_CLASS_IMPLEMENTATION(DState, boost::serialization::object_serializable);
BOOST_CLASS_TRACKING(DState,boost::serialization::track_never);

#endif
