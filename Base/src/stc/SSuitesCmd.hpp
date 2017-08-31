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
   SSuitesCmd(AbstractServer* as );
   SSuitesCmd() : ServerToClientCmd() {}

   void init(AbstractServer* as);
   virtual std::ostream& print(std::ostream& os) const;
   virtual bool equals(ServerToClientCmd*) const;
   virtual bool handle_server_response( ServerReply& server_reply, Cmd_ptr cts_cmd, bool debug ) const;
   virtual void cleanup() { std::vector<std::string>().swap(suites_);} /// run in the server, after command send to client

private:
   std::vector<std::string> suites_;

   friend class boost::serialization::access;
   template<class Archive>
   void serialize( Archive & ar, const unsigned int /*version*/ ) {
      ar & boost::serialization::base_object< ServerToClientCmd >( *this );
      ar & suites_;
   }
};

std::ostream& operator<<(std::ostream& os, const SSuitesCmd&);

#endif
