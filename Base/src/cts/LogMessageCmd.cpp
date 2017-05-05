/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #14 $ 
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

#include "ClientToServerCmd.hpp"
#include "AbstractServer.hpp"
#include "AbstractClientEnv.hpp"
#include "Log.hpp"
#include "Stats.hpp"
#include "CtsApi.hpp"

using namespace ecf;
using namespace std;
using namespace boost;
namespace po = boost::program_options;

std::ostream& LogMessageCmd::print(std::ostream& os) const
{
	return user_cmd(os,CtsApi::logMsg(msg_));
}

bool LogMessageCmd::equals(ClientToServerCmd* rhs) const
{
	LogMessageCmd* the_rhs = dynamic_cast< LogMessageCmd* > ( rhs );
	if ( !the_rhs ) return false;
	if (msg_ != the_rhs->msg()) return false;
 	return UserCmd::equals(rhs);
}

STC_Cmd_ptr LogMessageCmd::doHandleRequest(AbstractServer* as) const
{
	// ***** No need to log message here, already done via print, in base ****
	as->update_stats().log_msg_cmd_++;
	return PreAllocatedReply::ok_cmd();
}

const char* LogMessageCmd::arg() { return CtsApi::logMsgArg();}
const char* LogMessageCmd::desc() {
	return
	         "Writes the input string to the log file.\n"
	         "  arg1 = string\n"
	         "Usage:\n"
	         "  --msg=\"place me in the log file\""
	         ;
}

void LogMessageCmd::addOption(boost::program_options::options_description& desc) const {
	desc.add_options()( LogMessageCmd::arg(), po::value< string >(), LogMessageCmd::desc() );
}

void LogMessageCmd::create( Cmd_ptr& cmd,
						boost::program_options::variables_map& vm,
						AbstractClientEnv* ace) const
{
	string msg = vm[ arg() ].as< string >();
	if (ace->debug()) { cout << "  LogMessageCmd::create arg = " << msg << "\n";}
	cmd = Cmd_ptr( new LogMessageCmd( msg ) );
}

std::ostream& operator<<(std::ostream& os, const LogMessageCmd& c) { return c.print(os); }
