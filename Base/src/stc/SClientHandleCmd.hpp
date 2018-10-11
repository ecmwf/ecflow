#ifndef SCLIENT_HANDLE_CMD_HPP_
#define SCLIENT_HANDLE_CMD_HPP_
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #7 $ 
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
#include "ServerToClientCmd.hpp"

class SClientHandleCmd : public ServerToClientCmd {
public:
   explicit SClientHandleCmd(int handle) : handle_(handle) {}
	SClientHandleCmd() : ServerToClientCmd(){}

	void init(int handle) { handle_ = handle; }
	std::ostream& print(std::ostream& os) const override { return os << "cmd:SClientHandleCmd [ " << handle_ << " ]";}
	bool equals(ServerToClientCmd*) const override;
  	bool handle_server_response( ServerReply&, Cmd_ptr cts_cmd, bool debug ) const override;

private:
 	int handle_{0};

   friend class cereal::access;
   template<class Archive>
   void serialize(Archive & ar, std::uint32_t const version )
   {
 		ar(cereal::base_class< ServerToClientCmd >( this ),
 		   CEREAL_NVP(handle_));
  	}
};

std::ostream& operator<<(std::ostream& os, const SClientHandleCmd&);

#endif
