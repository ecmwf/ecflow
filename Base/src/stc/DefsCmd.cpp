/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        : Cmd
// Author      : Avi
// Revision    : $Revision: #27 $ 
//
// Copyright 2009-2012 ECMWF. 
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
	init(as,save_edit_history);
}

void DefsCmd::init(AbstractServer* as,bool save_edit_history)
{
   defs_ = as->defs();
   /// Return the current value of the state change no. So the that
   /// the next call to get the SSYncCmd , we need only return what's changed
   defs_->set_state_change_no( Ecf::state_change_no() );
   defs_->set_modify_change_no( Ecf::modify_change_no() );
   defs_->save_edit_history(save_edit_history);
}

bool DefsCmd::equals(ServerToClientCmd* rhs) const
{
	DefsCmd* the_rhs = dynamic_cast<DefsCmd*>(rhs);
	if (!the_rhs) return false;
	if (!ServerToClientCmd::equals(rhs)) return false;

	if (defs_ == NULL && the_rhs->defs() == NULL) return true;
	if (defs_ == NULL && the_rhs->defs() != NULL) return false;
	if (defs_ != NULL && the_rhs->defs() == NULL) return false;
	return (*defs_ == *(the_rhs->defs()));
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

	if ( !defs_.get() ) {
		std::stringstream ss;
		ss << "DefsCmd::handle_server_response: Error Node tree could not be retrieved from server. Request "; cts_cmd->print(ss); ss << " failed.\n";
		throw std::runtime_error(ss.str());
 	}

	if (server_reply.cli() && !cts_cmd->group_cmd()) {
		/// This Could be part of a group command, hence ONLY show defs if NOT group command
	   PrintStyle style(cts_cmd->show_style());

	   if (cts_cmd->show_style() != PrintStyle::MIGRATE) {
	      /// Auto generate externs, before writing to standard out. This can be expensive since
	      /// All the trigger references need to to be resolved. & AST need to be created first
	      /// The old spirit based parsing, horrendously, slow. Can't use Spirit QI, till IBM pull support it
	      defs_->auto_add_externs();
	   }
		std::cout << *defs_;
	}
	else {
		server_reply.set_sync( true );         // always in sync when getting the full defs
		server_reply.set_full_sync( true );    // Done a full sync, as opposed to incremental
		server_reply.set_client_defs( defs_ );
	}
	return true;
}


std::ostream& operator<<(std::ostream& os, const DefsCmd& c)       { return c.print(os); }
