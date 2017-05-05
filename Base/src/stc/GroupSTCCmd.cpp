/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #18 $ 
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

#include <boost/foreach.hpp>
#include "GroupSTCCmd.hpp"
#include "ClientToServerCmd.hpp"
#include "Str.hpp"
#include "Defs.hpp"
#include "Log.hpp"
#include "PrintStyle.hpp"
#include "WhyCmd.hpp"
#include "Task.hpp"
#include "Family.hpp"
#include "Suite.hpp"

using namespace ecf;
using namespace std;
using namespace boost;

std::ostream& GroupSTCCmd::print(std::ostream& os) const
{
 	return os << "cmd:GroupSTCCmd";
}

bool GroupSTCCmd::equals(ServerToClientCmd* rhs) const
{
	GroupSTCCmd* the_rhs = dynamic_cast< GroupSTCCmd* > ( rhs );
	if ( !the_rhs ) return false;

 	const std::vector<STC_Cmd_ptr>& rhsCmdVec = the_rhs->cmdVec();
	if (cmdVec_.size() != rhsCmdVec.size()) return false;

	for(size_t i = 0; i < cmdVec_.size(); i++) {
		if ( !cmdVec_[i]->equals( rhsCmdVec[i].get() ) ) {
			return false;
		}
	}

	return ServerToClientCmd::equals(rhs);
}

bool GroupSTCCmd::handle_server_response( ServerReply& server_reply, Cmd_ptr cts_cmd, bool debug ) const
{
	if (debug) std::cout << "  GroupSTCCmd::handle_server_response\n";

	BOOST_FOREACH(STC_Cmd_ptr subCmd, cmdVec_) {
		(void)subCmd->handle_server_response(server_reply, cts_cmd, debug);
  	}

	/// This assumes the DefsCmd::handle_server_response() | SNodeCmd::handle_server_response has been called
	/// this will populate ServerReply with the defs/node returned from the server
   defs_ptr defs = server_reply.client_defs();
   node_ptr node = server_reply.client_node();

	if (  defs.get() || node.get() ) {
		if (debug) std::cout << "   GroupSTCCmd::handle_server_response *get* | *sync* | *sync_full* called\n";

		/// client --group="get; show"         # where get will call DefsCmd will return defs, from the server
      /// client --group="get; show state"   # where get will call DefsCmd will return defs, from the server
      /// client --group="get /s1; show state"   # where get will call DefsCmd will return defs, from the server
      /// client --group="sync_full; show"       # similar to get return defs, from the server
      /// client --group="sync 1 0 0; show"      # where sync will call SyncCmd will return defs, from the server
		///                                        # will return those suites with handle 1

		// Print out the data that was received from server. as a part of get request.
		// The server can not do a show, it MUST be done at the Client side
		// The show request is only valid if the out bound request to the server
		PrintStyle::Type_t style = cts_cmd->show_style();
		if ( style != PrintStyle::NOTHING ) {
			if (debug) std::cout << "   GroupSTCCmd::handle_server_response *show* was called " << PrintStyle::to_string(style) << "\n";
			PrintStyle::setStyle(style);
			if (defs.get()) {

		      /// Auto generate externs, before writing to standard out. This can be expensive since
		      /// All the trigger references need to to be resolved. & AST need to be created first
		      /// The old spirit based parsing is horrendously, slow. Can't use Spirit QI, till IBM support it
		      if (cts_cmd->show_style() != PrintStyle::MIGRATE) {
 		         defs->auto_add_externs();
		      }

			   std::cout << *defs.get();
			}
			else   {
             if (node.get()) {
               Suite* suite = node->isSuite();
               if (suite) std::cout << *suite << "\n";
               Family* fam = node->isFamily();
               if (fam) std::cout << *fam << "\n";
               Task* task = node->isTask();
               if (task) std::cout << *task << "\n";
            }
			}
		}
	}

	std::string nodePath;
	if (cts_cmd->why_cmd(nodePath) && defs.get()) {
		if (debug) std::cout << "  GroupSTCCmd::handle_server_response *why* was called\n";

		/// client --group="get; why"          # where get will call DefsCmd will return defs, from the server
		/// client --group="get; why <path>"   # where get will call DefsCmd will return defs, from the server
		WhyCmd cmd(defs, nodePath);
		std::cout << cmd.why() << "\n";
 	}

	return true;
}

void GroupSTCCmd::addChild(STC_Cmd_ptr childCmd)
{
	LOG_ASSERT(childCmd.get(),""); // Dont add NULL children
	cmdVec_.push_back(childCmd);
}

std::ostream& operator<<(std::ostream& os, const GroupSTCCmd& c)   { return c.print(os); }
