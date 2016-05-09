#ifndef S_STATE_HPP_
#define S_STATE_HPP_

/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #6 $ 
//
// Copyright 2009-2016 ECMWF. 
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description :
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
#include <string>
#include <boost/noncopyable.hpp>

class SState : private boost::noncopyable {
public:
   /// The following table shows the effect of state, on server behaviour:
   ///
   ///           User Request    Task Request   Job Scheduling   Check-pointing
   /// RUNNING      yes               yes              yes            yes
   /// SHUTDOWN     yes               yes              no             yes
   /// HALTED       yes               no               no             no
	enum State {
	   HALTED,
 		SHUTDOWN,
 		RUNNING
 	};

	/// Given an integer return the server state as a string.
	/// if int is not  0,1,2 return "UNKNOWN
	static std::string to_string(int status);
	static std::string to_string(SState::State);

   static SState::State toState(const std::string& state);
   static bool isValid(const std::string& state);

private:
	SState();
};
#endif
