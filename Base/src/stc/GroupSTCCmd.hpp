#ifndef GROUP_STC_CMD_HPP_
#define GROUP_STC_CMD_HPP_
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #8 $ 
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

class GroupSTCCmd : public ServerToClientCmd {
public:
	GroupSTCCmd() : ServerToClientCmd() {}

 	std::ostream& print(std::ostream& os) const override;
  	bool equals(ServerToClientCmd*) const override;
  	bool handle_server_response( ServerReply& server_reply, Cmd_ptr cts_cmd, bool debug ) const override;

   void addChild(STC_Cmd_ptr childCmd);
 	const std::vector<STC_Cmd_ptr>& cmdVec() const { return cmdVec_;}

private:
 	std::vector<STC_Cmd_ptr> cmdVec_;

   friend class cereal::access;
   template<class Archive>
   void serialize(Archive & ar, std::uint32_t const version )
   {
 		ar(cereal::base_class< ServerToClientCmd >( this ),
 		   CEREAL_NVP(cmdVec_));
   }
};

std::ostream& operator<<(std::ostream& os, const GroupSTCCmd&);

#endif
