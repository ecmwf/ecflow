#ifndef SSUITES_CMD_HPP_
#define SSUITES_CMD_HPP_
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
class AbstractServer;

//================================================================================
// Paired with CtsCmd(SUITES)
// Client---(CtsCmd(SUITES))---->Server-----(SSuitesCmd)--->client:
//================================================================================
class SSuitesCmd : public ServerToClientCmd {
public:
   explicit SSuitesCmd(AbstractServer* as );
   SSuitesCmd() : ServerToClientCmd() {}

   void init(AbstractServer* as);
   std::ostream& print(std::ostream& os) const override;
   bool equals(ServerToClientCmd*) const override;
   bool handle_server_response( ServerReply& server_reply, Cmd_ptr cts_cmd, bool debug ) const override;
   void cleanup() override { std::vector<std::string>().swap(suites_);} /// run in the server, after command send to client

private:
   std::vector<std::string> suites_;

   friend class cereal::access;
   template<class Archive>
   void serialize(Archive & ar, std::uint32_t const version )
   {
      ar(cereal::base_class< ServerToClientCmd >( this ),
         CEREAL_NVP(suites_));
   }
};

std::ostream& operator<<(std::ostream& os, const SSuitesCmd&);

#endif
