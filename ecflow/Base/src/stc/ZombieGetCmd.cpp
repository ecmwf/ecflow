/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #11 $ 
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
#include "ZombieGetCmd.hpp"
#include "AbstractServer.hpp"

using namespace std;
using namespace boost;

ZombieGetCmd::ZombieGetCmd(AbstractServer* as)
{
	init(as);
}

// called in the server
void ZombieGetCmd::init(AbstractServer* as)
{
	zombies_.clear();
	as->zombie_ctrl().get(zombies_);
}

bool ZombieGetCmd::equals(ServerToClientCmd* rhs) const
{
	ZombieGetCmd* the_rhs = dynamic_cast<ZombieGetCmd*>(rhs);
	if (!the_rhs) return false;
	return ServerToClientCmd::equals(rhs);
}

std::ostream& ZombieGetCmd::print(std::ostream& os) const
{
	os << "cmd:ZombieGetCmd [ " << zombies_.size() << " ]";
	return os;
}

// Called in client
bool ZombieGetCmd::handle_server_response( ServerReply& server_reply, Cmd_ptr cts_cmd,  bool debug ) const
{
	if (debug) {
		std::cout << "ZombieGetCmd::handle_server_response zombies.size() = " << zombies_.size() << "\n";
	}

	if (server_reply.cli()) {
      std::cout << Zombie::pretty_print(zombies_);
	}
	else {
	   if (debug) {
	      std::cout << Zombie::pretty_print(zombies_);
	   }
		server_reply.set_zombies( zombies_);
	}
	return true;
}

std::ostream& operator<<(std::ostream& os, const ZombieGetCmd& c) { return c.print(os); }
