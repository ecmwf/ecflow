#ifndef PASSWD_HPP_
#define PASSWD_HPP_

//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #4 $ 
//
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description :
//    The tasks send by the ECF will have GENERATED PASSWORD that is not
//    stored into the any file (except the job file). It only exist in the
//    ECF_server-memory. If the ECF server fails and a new one is started, it must be
//    told (by operator) to accept the messages from the unknown jobs.
//
//    The GENERATED PASSWORDs are stored into the CHECKPOINT file. So if the
//    ECF fails and is RESTARTed from a valid CHECKPOINT file, the ECF gets
//    to know about the tasks.
//
//    The user passwords are kept in a file. When ever an user logs into
//    the ECF or changes the "level" of privileges while already in the
//    file is consulted.
//============================================================================

#include <string>

class Passwd {
public:

	/// generate a random password
	static std::string generate();

private:
	Passwd() = delete;
	Passwd(const Passwd&) = delete;
	const Passwd& operator=(const Passwd&) = delete;
};

#endif
