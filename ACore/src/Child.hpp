#ifndef CHILD_HPP_
#define CHILD_HPP_
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #5 $ 
//
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description : Specifies the different kinds of child commands
//               These are specified in the job file, and communicate with the server
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
#include <string>
#include <vector>
#include <boost/noncopyable.hpp>

namespace ecf {

class Child : private boost::noncopyable {
public:
	enum CmdType    { INIT, EVENT, METER, LABEL, WAIT, QUEUE, ABORT, COMPLETE };

	enum ZombieType {
	   USER,           // zombie created by user action
	   ECF,            // two init commands, or aborted and complete, and received any other child command
	   ECF_PID,        // pid miss-match, but password matches,  -> same job submitted twice |
	   ECF_PASSWD,     // password miss-match, but pid matches   -> WTF, user edited ECF_PASS in job file ?
	   ECF_PID_PASSWD, // pid and password missmatch             -> Job re-queued and submitted again
	   PATH,           // zombie, because path to task does not exist in the server
	   NOT_SET
	};

	static std::string to_string(ZombieType);
	static bool valid_zombie_type( const std::string& );
	static ZombieType zombie_type( const std::string&);

	static std::string to_string(const std::vector<Child::CmdType>&);
	static std::string to_string( Child::CmdType );
	static std::vector<Child::CmdType> child_cmds(const std::string&);
	static Child::CmdType child_cmd(const std::string&);

	/// Expect a , separated string
	static bool valid_child_cmds( const std::string& );
	static bool valid_child_cmd( const std::string& );

private:
	Child();
};


class User : private boost::noncopyable {
public:
	enum Action   { FOB, FAIL, ADOPT, REMOVE, BLOCK, KILL };

	static bool valid_user_action( const std::string& );
	static Action user_action( const std::string& );
	static std::string to_string(Action);

private:
	User();
};

}
#endif
