#ifndef SCLIENT_HANDLE_CMD_HPP_
#define SCLIENT_HANDLE_CMD_HPP_
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #7 $ 
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

class SClientHandleCmd : public ServerToClientCmd {
public:
	SClientHandleCmd(int handle) : handle_(handle) {}
	SClientHandleCmd() : ServerToClientCmd() , handle_(0) {}

	void init(int handle) { handle_ = handle; }
	virtual std::ostream& print(std::ostream& os) const { return os << "cmd:SClientHandleCmd [ " << handle_ << " ]";}
	virtual bool equals(ServerToClientCmd*) const;
  	virtual bool handle_server_response( ServerReply&, Cmd_ptr cts_cmd, bool debug ) const;

private:
 	int handle_;

	friend class boost::serialization::access;
	template<class Archive>
	void serialize( Archive & ar, const unsigned int /*version*/ ) {
 		ar & boost::serialization::base_object< ServerToClientCmd >( *this );
 		ar & handle_;
  	}
};

std::ostream& operator<<(std::ostream& os, const SClientHandleCmd&);

#endif
