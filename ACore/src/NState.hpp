#ifndef NSTATE_HPP_
#define NSTATE_HPP_

//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #16 $ 
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
#include "Serialization.hpp"

// NState: stores the state of a node.
// *The class NState just used to define the enum, however we also
// needed to know when the state changed. Hence the use of state_change_no
// Uses default copy constructor and destructor, and equality
//
// The default_state() should *NEVER* change as it will affect client/server comms
//
class NState {
public:
	enum State { UNKNOWN =0, COMPLETE=1,  QUEUED=2, ABORTED=3, SUBMITTED=4, ACTIVE=5 };
	explicit NState(State s): st_(s), state_change_no_(0) {}
	NState(): st_(default_state()),state_change_no_(0) {}
   static NState::State default_state() { return NState::UNKNOWN; }

	State state() const { return st_;}
	void setState(State);

	// The state_change_no is never reset. Must be incremented if it can affect equality
 	unsigned int state_change_no() const { return state_change_no_; }

	bool operator==(const NState& rhs) const { return st_ == rhs.st_;}
	bool operator!=(const NState& rhs) const { return st_ != rhs.st_;}
	bool operator==(State s) const { return s == st_;}
	bool operator!=(State s) const { return s != st_;}

   static const char* toString(NState::State s);
   static const char* to_html(NState::State s);
	static const char* toString(const NState& ns) { return toString(ns.state());}
	static NState::State toState(const std::string&);
	static bool isValid(const std::string& state);
	static std::vector<std::string> allStates();
	static std::vector<NState::State> states();

private:
	State st_;
	unsigned int state_change_no_;  // *not* persisted, only used on server side

   friend class cereal::access;
	template<class Archive>
	void serialize(Archive & ar, std::uint32_t const version )
	{
	   ar(CEREAL_NVP(st_));
	}
};

#endif
