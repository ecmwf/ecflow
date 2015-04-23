//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef CONNECTSTATE_HPP_
#define CONNECTSTATE_HPP_

#include <string>
#include <ctime>

class ConnectState
{
public:
	ConnectState();

	enum State {Undef,Normal,Disconnected,InitFailed,Lost};

	void state(State state) {state_=state;}
	State state() const {return state_;}

	const std::string& describe() const;

	void lapStart();
	void lapStop();
	void errorMessage(const std::string);
	std::time_t startTime() const {return lapStart_;}
	std::time_t stopTime() const {return lapStop_;}
	const std::string& errorMessage() const {return errorMessage_;}

protected:
	void init();

	State state_;
	std::time_t lapStart_;
	std::time_t lapStop_;
	std::string errorMessage_;
};

#endif
