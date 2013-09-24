#ifndef ZOMBIE_GET_CMD_HPP_
#define ZOMBIE_GET_CMD_HPP_
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #4 $ 
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

#include "ServerToClientCmd.hpp"
#include "Zombie.hpp"
class AbstractServer;

//================================================================================
// Paired with CtsCmd(GET_ZOMBIES)
// Client---CtsCmd(GET_ZOMBIES)---->Server-----(ZombieGetCmd)--->client:
//================================================================================
class ZombieGetCmd : public ServerToClientCmd {
public:
	ZombieGetCmd(AbstractServer*);
	ZombieGetCmd() : ServerToClientCmd() {}

	void init(AbstractServer*);
  	virtual bool handle_server_response( ServerReply&, Cmd_ptr cts_cmd, bool debug ) const;
  	virtual std::ostream& print(std::ostream& os) const;
	virtual bool equals(ServerToClientCmd*) const;

private:
	std::vector<Zombie> zombies_;

	friend class boost::serialization::access;
	template<class Archive>
	void serialize( Archive & ar, const unsigned int /*version*/ ) {
 		ar & boost::serialization::base_object< ServerToClientCmd >( *this );
 		ar & zombies_;
  	}
};

std::ostream& operator<<(std::ostream& os, const ZombieGetCmd&);

#endif
