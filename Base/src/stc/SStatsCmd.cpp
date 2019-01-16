/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #11 $ 
//
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description :
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8

#include "SStatsCmd.hpp"
#include "AbstractServer.hpp"
#include "Defs.hpp"

using namespace std;

//////////////////////////////////////////////////////////////////////////////////////////////////

SStatsCmd::SStatsCmd(AbstractServer* as )
{
	init(as);
}

void SStatsCmd::init(AbstractServer* as)
{
   as->stats().update_for_serialisation();
	stats_ = as->stats();
 	stats_.no_of_suites_ = as->defs()->suiteVec().size();
}

bool SStatsCmd::equals(ServerToClientCmd* rhs) const
{
	auto* the_rhs = dynamic_cast<SStatsCmd*>(rhs);
	if (!the_rhs) return false;
	return ServerToClientCmd::equals(rhs);
}

std::ostream& SStatsCmd::print(std::ostream& os) const
{
	os << "cmd:SStatsCmd ";
	return os;
}

bool SStatsCmd::handle_server_response( ServerReply& server_reply, Cmd_ptr /*cts_cmd*/, bool debug ) const
{
	if (debug) std::cout << "  SStatsCmd::handle_server_response\n";
   if (server_reply.cli())  stats_.show();
   else { server_reply.set_stats( stats_ ); }
	return true;
}

std::ostream& operator<<(std::ostream& os, const SStatsCmd& c) { return c.print(os); }
