//============================================================================
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
//============================================================================
#include <cassert>
#include <fstream>
#include <sstream>
#include <stdexcept>

#include "ServerToClientResponse.hpp"
#include "ClientToServerRequest.hpp" // for throw
#include "AbstractServer.hpp"

using namespace std;
using namespace ecf;

bool ServerToClientResponse::handle_server_response( ServerReply& r, Cmd_ptr cts_cmd, bool debug ) const
{
	/// Called in client context: see ClientInvoker
	if (stc_cmd_.get()) {
		return stc_cmd_->handle_server_response(r,cts_cmd,debug);
 	}

    /// ClientToServerRequest::handleRequest returned a NULL pointer stc_cmd_.
	std::stringstream ss;
	ss << "ServerToClientResponse::handle_server_response: ";
	if (cts_cmd.get()) {
		ss << "Client request ";
		cts_cmd->print(ss);
		ss << " failed. ";
	}
	ss << "Server replied with a NULL message\n";
 	throw std::runtime_error(ss.str());
}

std::ostream& ServerToClientResponse::print( std::ostream& os ) const
{
	if (stc_cmd_.get()) {
		return stc_cmd_->print(os);
	}
	return os << "NULL ServerToClientResponse";
}

bool ServerToClientResponse::operator==(const ServerToClientResponse& rhs) const
{
	if (!stc_cmd_.get() && !rhs.stc_cmd_.get()) {
		return true;
	}
	if (stc_cmd_.get() && !rhs.stc_cmd_.get()) {
		return false;
	}
	if (!stc_cmd_.get() && rhs.stc_cmd_.get()) {
		return false;
	}
	return (stc_cmd_->equals(rhs.stc_cmd_.get()));
}

std::ostream& operator<<( std::ostream& os, const ServerToClientResponse& d ) {
	return d.print( os );
}
