/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #31 $ 
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
#include "SNewsCmd.hpp"
#include "Defs.hpp"
#include "Ecf.hpp"
#include "AbstractServer.hpp"
#include "Log.hpp"

using namespace std;
using namespace ecf;

/// Custom handling of command logging so that we can add additional debug.
/// Rely's on CSyncCmd not adding newline when logging the command
#define DEBUG_NEWS 1


SNewsCmd::SNewsCmd( unsigned int client_handle,
                    unsigned int client_state_change_no,
                    unsigned int client_modify_change_no,
                    AbstractServer* as)
: news_(ServerReply::NO_NEWS)
{
   init(client_handle,client_state_change_no,client_modify_change_no,as);
}

/// Called in the server
void SNewsCmd::init(
		unsigned int client_handle, // a reference to a set of suites used by client
		unsigned int client_state_change_no,
		unsigned int client_modify_change_no, AbstractServer* as)
{
	news_ = ServerReply::NO_NEWS;

 	// =====================================================================================
 	// The code to determine changes here must also relate to SSyncCmd
 	// ======================================================================================

	if (client_handle == 0) {

		// Here Ecf::modify_change_no() and  Ecf::state_change_no() represent the max change numbers over *all* the suites

	   /// *** The client_modify_change_no and client_state_change_no should always be trailing the server
	   /// *** i.e the value should be less or equal to server. However if::
	   /// ***   o/ Server **dies** we can get the case, where client numbers are greater than server numbers.
	   /// ***   o/ Server changes number overflows, since it unsigned, and re-start's with 0
	   /// *** When no handle are involved, we can get by with a full sync
	   /// *** Note: whenever the server starts, the state and modify numbers start from zero
	   if ( (client_modify_change_no > Ecf::modify_change_no()) || (client_state_change_no > Ecf::state_change_no())) {

         news_ = ServerReply::DO_FULL_SYNC;

#if DEBUG_NEWS
	      std::stringstream ss;
	      ss << " [server(" << Ecf::state_change_no() << "," << Ecf::modify_change_no() << ") : client no > server no ! :DO_FULL_SYNC]";
	      log_append(ss.str());
#else
	      log_append("");
#endif
         return;
	   }

		if ( client_modify_change_no < Ecf::modify_change_no()) {

		   news_ = ServerReply::NEWS;

#if DEBUG_NEWS
         std::stringstream ss;
         ss << " [server(" << Ecf::state_change_no() << "," << Ecf::modify_change_no() << ") : *Large* scale changes :NEWS]";
         log_append(ss.str());
#else
         log_append("");
#endif
			return;
		}

		if ( client_state_change_no < Ecf::state_change_no() ) {

		   news_ = ServerReply::NEWS;

#if DEBUG_NEWS
         std::stringstream ss;
         ss << " [server(" << Ecf::state_change_no() << "," << Ecf::modify_change_no() << ") : *Small* scale changes :NEWS]";
         log_append( ss.str());
#else
         log_append("");
#endif
         return;
		}


#if DEBUG_NEWS
   log_append(" [:NO_NEWS]");
#else
   log_append("");
#endif
		return;
	}

	// =============================================================================================
	// Handle used: Determine the max modify and state change no, for suites in our handle
	// =============================================================================================

	/// *** If we can't find the handle, then it may be that the server died ?
	/// *** It is up to the client to do a full sync *including* re-registering suites
	ClientSuiteMgr& client_suite_mgr = as->defs()->client_suite_mgr();
	if ( ! client_suite_mgr.valid_handle(client_handle)) {

	   news_ = ServerReply::DO_FULL_SYNC;

#if DEBUG_NEWS
      std::stringstream ss;
      ss << " [server(" << Ecf::state_change_no() << "," << Ecf::modify_change_no() << ") : Can not find handle(" << client_handle << ") :DO_FULL_SYNC]";
      log_append(ss.str());
#else
      log_append("");
#endif

	   return;
	}

	/// *** The client_modify_change_no and client_state_change_no should always be trailing the server
	/// *** i.e the value should be less or equal to server. However if
   /// ***   o/ Server **dies** we can get the case, where client numbers are greater than server numbers.
   /// ***   o/ Server changes number overflows, since it unsigned, and re-start's with 0
	/// *** we can get the case, where client numbers are greater than server numbers and also the
	/// *** handle will not exist in the server,
	/// *** It is up to the client to do a full sync *including* re-registering suites
	/// *** Note: whenever the server starts, the state and modify numbers start from zero
	unsigned int max_client_handle_modify_change_no = 0;
	unsigned int max_client_handle_state_change_no = 0;
	client_suite_mgr.max_change_no( client_handle,max_client_handle_state_change_no,max_client_handle_modify_change_no);

	if ((client_modify_change_no > max_client_handle_modify_change_no) || (client_state_change_no > max_client_handle_state_change_no)) {

	   news_ = ServerReply::DO_FULL_SYNC;

#if DEBUG_NEWS
      std::stringstream ss;
      ss << " [server handle(" << max_client_handle_state_change_no << ","
         << max_client_handle_modify_change_no << ")  server(" << Ecf::state_change_no() << "," << Ecf::modify_change_no()
         << ") : client no > server no ! :DO_FULL_SYNC]";
      log_append(ss.str());
#else
      log_append("");
#endif

	   return;
	}

	/// Changes where user adds a new handle/auto adds/removes require a full update, but only for changed handle
	if (client_suite_mgr.handle_changed( client_handle )) {

	   news_ = ServerReply::NEWS;

#if DEBUG_NEWS
      std::stringstream ss;
      ss << " [server handle(" << max_client_handle_state_change_no << ","
         << max_client_handle_modify_change_no << ") server(" << Ecf::state_change_no() << "," << Ecf::modify_change_no()
         << ") : *Large* scale changes (new handle or suites added or removed) :NEWS]";
      log_append(ss.str());
#else
      log_append("");
#endif

	   return;
	}

	// The client handle represents a subset of the suites.
	if ( client_modify_change_no < max_client_handle_modify_change_no) {

  	   news_ = ServerReply::NEWS;

#if DEBUG_NEWS
      std::stringstream ss;
      ss << " [server handle(" << max_client_handle_state_change_no << ","
         << max_client_handle_modify_change_no << ") server(" << Ecf::state_change_no() << "," << Ecf::modify_change_no()
         << ") : *Large* scale changes :NEWS]";
      log_append( ss.str());
#else
      log_append("");
#endif

	   return;
	}

	// This should also reflect changes made just to the defs(state/suspended) and also the server state
	if ( client_state_change_no < max_client_handle_state_change_no ) {

	   news_ = ServerReply::NEWS;

#if DEBUG_NEWS
      std::stringstream ss;
      ss << " [server handle(" << max_client_handle_state_change_no << ","
         << max_client_handle_modify_change_no << ") server(" << Ecf::state_change_no() << "," << Ecf::modify_change_no()
         << ") : *Small* scale changes :NEWS]";
      log_append(ss.str());
#else
      log_append("");
#endif

      return;
	}

#if DEBUG_NEWS
   log_append(" [:NO_NEWS]");
#else
   log_append("");
#endif
}


/// Called in the client
bool SNewsCmd::handle_server_response( ServerReply& server_reply, Cmd_ptr /*cts_cmd*/, bool debug ) const
{
	if (debug) std::cout << "  SNewsCmd::handle_server_response news_ = " << news_ << "\n";
	server_reply.set_news( news_);
	return true;
}

bool SNewsCmd::equals(ServerToClientCmd* rhs) const
{
	SNewsCmd* the_rhs = dynamic_cast<SNewsCmd*>(rhs);
	if (!the_rhs) return false;
	if (news_ != the_rhs->news()) return false;
	return ServerToClientCmd::equals(rhs);
}

std::ostream& SNewsCmd::print(std::ostream& os) const
{
	os << "cmd:SNewsCmd [ " << news_ << " ] ";
 	return os;
}

std::ostream& operator<<(std::ostream& os, const SNewsCmd& c)  { return c.print(os); }
