#ifndef NSTATE_HPP_
#define NSTATE_HPP_

//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #16 $ 
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

#include <string>
#include <vector>
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/level.hpp>
#include <boost/serialization/tracking.hpp>

// NState: stores the state of a node.
// *The class NState just used to define the enum, however we also
// needed to know when the state changed. Hence the use of state_change_no
// Uses default copy constructor and destructor, and equality
class NState {
public:
	enum State { UNKNOWN =0, COMPLETE=1,  QUEUED=2, ABORTED=3, SUBMITTED=4, ACTIVE=5 };
	NState(State s): state_(s), state_change_no_(0) {}
	NState(): state_(UNKNOWN),state_change_no_(0) {}

	State state() const { return state_;}
	void setState(State);

	// The state_change_no is never reset. Must be incremented if it can affect equality
 	unsigned int state_change_no() const { return state_change_no_; }

	bool operator==(const NState& rhs) const { return state_ == rhs.state_;}
	bool operator!=(const NState& rhs) const { return state_ != rhs.state_;}
	bool operator==(State s) const { return s == state_;}
	bool operator!=(State s) const { return s != state_;}

 	static const char* toString(NState::State s);
	static const char* toString(const NState& ns) { return toString(ns.state());}
	static NState::State toState(const std::string& state);
	static bool isValid(const std::string& state);
	static std::vector<std::string> allStates();
	static std::vector<NState::State> states();

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

// This should ONLY be added to objects that are *NOT* serialised through a pointer
BOOST_CLASS_IMPLEMENTATION(NState, boost::serialization::object_serializable);
BOOST_CLASS_TRACKING(NState,boost::serialization::track_never);

#endif
