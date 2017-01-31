/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #3 $ 
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
#include "SStringCmd.hpp"

using namespace std;
using namespace boost;

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SStringCmd::equals(ServerToClientCmd* rhs) const
{
	SStringCmd* the_rhs = dynamic_cast<SStringCmd*>(rhs);
	if (!the_rhs) return false;
	if (str_ != the_rhs->get_string()) return false;
	return ServerToClientCmd::equals(rhs);
}

std::ostream& SStringCmd::print(std::ostream& os) const
{
	os << "cmd:SStringCmd ";
	return os;
}

bool SStringCmd::handle_server_response( ServerReply& server_reply, Cmd_ptr cts_cmd, bool debug ) const
{
	if (debug) cout << "  SStringCmd::handle_server_response str.size()= " << str_.size() << "\n";
	if (server_reply.cli())  std::cout << str_ << "\n";
	else server_reply.set_string( str_ );
	return true;
}

std::ostream& operator<<(std::ostream& os, const SStringCmd& c)      { return c.print(os); }
