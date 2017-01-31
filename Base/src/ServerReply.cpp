/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #10 $ 
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
//#include <iostream>
#include "ServerReply.hpp"

/// *Note* server_reply_.client_handle_ is kept until the next call to register a new client_handle
/// The client invoker can be used multiple times, hence keep value of defs, and client handle in server reply
void ServerReply::clear_for_invoke(bool command_line_interface)
{
   //std::cout << "ServerReply::clear_for_invoke\n";
	cli_ =  command_line_interface;
	in_sync_ = false;
	full_sync_ = false;
	news_ = ServerReply::NO_NEWS;
	block_client_on_home_server_ = false;
	block_client_server_halted_ = false;
	block_client_zombie_detected_ = false;
	host_.clear();
	port_.clear();
	error_msg_.clear();
	str_.clear();
	zombies_.clear();
	str_vec_.clear();
	client_handle_suites_.clear();
	changed_nodes_.clear();
}
