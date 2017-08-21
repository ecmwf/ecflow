/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        : Cmd
// Author      : Avi
// Revision    : $Revision: #27 $ 
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

#include "DefsCmd.hpp"
#include "ClientToServerCmd.hpp"
#include "Defs.hpp"
#include "Ecf.hpp"
#include "AbstractServer.hpp"
#include "PrintStyle.hpp"

using namespace std;
using namespace boost;

//=====================================================================================
// The defs command returns the full definition back to the client

DefsCmd::DefsCmd(AbstractServer* as,bool save_edit_history)
{
	init(as,save_edit_history); // save edit history
}

void DefsCmd::init(AbstractServer* as,bool save_edit_history)
{
   /// Return the current value of the state change no. So the that
   /// the next call to get the SSYncCmd , we need only return what's changed
   as->defs()->set_state_change_no( Ecf::state_change_no() );
   as->defs()->set_modify_change_no( Ecf::modify_change_no() );
   as->defs()->save_edit_history(save_edit_history);
   DefsCache::update_cache_if_state_changed(as->defs());
}

bool DefsCmd::equals(ServerToClientCmd* rhs) const
{
	DefsCmd* the_rhs = dynamic_cast<DefsCmd*>(rhs);
	if (!the_rhs) return false;
	if (!ServerToClientCmd::equals(rhs)) return false;
	return (full_server_defs_as_string_  == the_rhs->full_server_defs_as_string_);
}

std::ostream& DefsCmd::print(std::ostream& os) const
{
	os << "cmd:DefsCmd [ defs ]";
	return os;
}

// Called in client
bool DefsCmd::handle_server_response( ServerReply& server_reply, Cmd_ptr cts_cmd,  bool debug ) const
{
	if (debug) std::cout << "  DefsCmd::handle_server_response show_state = " << PrintStyle::to_string(cts_cmd->show_style()) << "\n";

	// If we asked for the defs node tree from the server, then this is what we should have got back.
	// ** Keep existing defs in memory, until a new one is requested. This allows clients
	// ** to continue using this defs, in between other api calls, until a new defs is requested.

   defs_ptr defs = DefsCache::restore_defs_from_string(full_server_defs_as_string_);

	if (server_reply.cli() && !cts_cmd->group_cmd()) {
		/// This Could be part of a group command, hence ONLY show defs if NOT group command
	   PrintStyle style(cts_cmd->show_style());

	   if (cts_cmd->show_style() != PrintStyle::MIGRATE) {
	      /// Auto generate externs, before writing to standard out. This can be expensive since
	      /// All the trigger references need to to be resolved. & AST need to be created first
	      /// The old spirit based parsing, horrendously, slow. Can't use Spirit QI, till IBM pull support it
	      defs->auto_add_externs();
	   }
		std::cout << *defs;
	}
	else {
		server_reply.set_sync( true );         // always in sync when getting the full defs
		server_reply.set_full_sync( true );    // Done a full sync, as opposed to incremental
		server_reply.set_client_defs( defs );
	}
	return true;
}


std::ostream& operator<<(std::ostream& os, const DefsCmd& c)       { return c.print(os); }
