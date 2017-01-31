/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #13 $ 
//
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description :
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
#include <iostream>
#include "ErrorCmd.hpp"
#include "ClientToServerCmd.hpp"
#include "Log.hpp"

using namespace ecf;
using namespace std;
using namespace boost;

ErrorCmd::ErrorCmd(const std::string& errorMsg)
{
	init(errorMsg);
}

void ErrorCmd::init( const std::string& errorMsg)
{
//#ifdef DEBUG
//	std::cout << ErrorCmd::init " << errorMsg << "\n";
//	LogToCout toCoutAsWell;
//#endif

	LOG_ASSERT(!errorMsg.empty(),"");
	error_msg_ = errorMsg;

	// Log the error, Remove any "/n" as the log file will add this automatically
	size_t pos = error_msg_.rfind("\n");
	if (pos != string::npos)  error_msg_.erase(error_msg_.begin() + pos);
	ecf::log(Log::ERR,error_msg_); // will automatically add end of line
}

std::ostream& ErrorCmd::print(std::ostream& os) const { return os << "cmd:Error [ " << error_msg_ << " ]";}

bool ErrorCmd::equals(ServerToClientCmd* rhs) const
{
	return (dynamic_cast<ErrorCmd*>(rhs)) ? ServerToClientCmd::equals(rhs) : false;
}

bool ErrorCmd::handle_server_response( ServerReply& server_reply, Cmd_ptr cts_cmd, bool debug ) const
{
	if (debug) std::cout << "  ErrorCmd::handle_server_response " << error_msg_ << "\n";

	std::stringstream ss;
	ss << "Error: request( "; cts_cmd->print(ss); ss << " ) failed!  Server replied with: '" <<  error_msg_ << "'\n";
	server_reply.set_error_msg(ss.str());
	return false;
}

std::ostream& operator<<(std::ostream& os, const ErrorCmd& c) { return c.print(os); }

