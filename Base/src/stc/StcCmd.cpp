/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        : Cmd
// Author      : Avi
// Revision    : $Revision: #7 $ 
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
#include <iostream>
#include "StcCmd.hpp"
#include "ClientToServerCmd.hpp"

std::ostream& StcCmd::print(std::ostream& os) const
{
	switch (api_) {
		case StcCmd::OK:                          return os << "cmd:Ok"; break;
		case StcCmd::BLOCK_CLIENT_SERVER_HALTED:  return os << "cmd:Server_halted"; break;
		case StcCmd::BLOCK_CLIENT_ON_HOME_SERVER: return os << "cmd:Wait"; break;
		case StcCmd::BLOCK_CLIENT_ZOMBIE:         return os << "cmd:Zombie"; break;
		case StcCmd::INVALID_ARGUMENT:            return os << "cmd:Invalid_argumnet"; break;
		default: assert(false); break;
 	}
	assert(false); // unknown command
	return os << "cmd:Unknown??";
}

// Client context
bool StcCmd::handle_server_response( ServerReply& server_reply, Cmd_ptr cts_cmd, bool debug ) const
{
	bool ret = false;
	switch (api_) {
		case StcCmd::OK: {
			if (debug) std::cout << "  StcCmd::handle_server_response OK\n";
			ret = true;
			break;
		}
 		case StcCmd::BLOCK_CLIENT_SERVER_HALTED: {
			if (debug) std::cout << "  StcCmd::handle_server_response BLOCK_CLIENT_SERVER_HALTED\n";
			server_reply.set_block_client_server_halted(); // requires further work, by ClientInvoker
			break;
 		}
		case StcCmd::BLOCK_CLIENT_ON_HOME_SERVER: {
			if (debug) std::cout << "  StcCmd::handle_server_response BLOCK_CLIENT_ON_HOME_SERVER\n";
			server_reply.set_block_client_on_home_server(); // requires further work, by ClientInvoker
			break;
 		}
		case StcCmd::BLOCK_CLIENT_ZOMBIE: {
			if (debug) std::cout << "  StcCmd::handle_server_response BLOCK_CLIENT_ZOMBIE\n";
			server_reply.set_block_client_zombie_detected(); // requires further work, by ClientInvoker
			break;
 		}
		case StcCmd::INVALID_ARGUMENT: {
			// This is created on the client side, after detecting a INVALID_ARGUMENT reply from the server
			// This keeps compatibility with 4 servers
			if (debug) std::cout << "  StcCmd::handle_server_response INVALID_ARGUMENT\n";
			server_reply.set_invalid_argument();// requires further work, by ClientInvoker
			std::stringstream ss;
			ss << "Error: request( "; cts_cmd->print(ss); ss << " ) failed! Server replied with: invalid_argument(Could not decode client protocol)\n";
			server_reply.set_error_msg(ss.str());
			break;
 		}
		default: assert(false); break;
 	}
	return ret;
}

bool StcCmd::equals(ServerToClientCmd* rhs) const
{
	StcCmd* the_rhs = dynamic_cast<StcCmd*>(rhs);
	if (!the_rhs) return false;
	if (api_ != the_rhs->api()) return false;
	return ServerToClientCmd::equals(rhs);
}

std::ostream& operator<<(std::ostream& os, const StcCmd& c)   { return c.print(os); }
