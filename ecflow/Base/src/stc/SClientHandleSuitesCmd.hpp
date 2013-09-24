#ifndef SCLIENT_HANDLE_SUITES_CMD_HPP_
#define SCLIENT_HANDLE_SUITES_CMD_HPP_
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
class AbstractServer;

//================================================================================
class SClientHandleSuitesCmd : public ServerToClientCmd {
public:
   SClientHandleSuitesCmd(AbstractServer* as );
   SClientHandleSuitesCmd() : ServerToClientCmd() {}

   void init(AbstractServer* as);
   virtual std::ostream& print(std::ostream& os) const;
   virtual bool equals(ServerToClientCmd*) const;
   virtual bool handle_server_response( ServerReply& server_reply, Cmd_ptr cts_cmd, bool debug ) const;

private:
   std::vector<std::pair<std::string,  std::vector<unsigned int> > > users_;          // users , list of handles
   std::vector<std::pair<unsigned int, std::vector<std::string> > >  client_handles_; // handle, list of suites

   friend class boost::serialization::access;
   template<class Archive>
   void serialize( Archive & ar, const unsigned int /*version*/ ) {
      ar & boost::serialization::base_object< ServerToClientCmd >( *this );
      ar & users_;
      ar & client_handles_;
   }
};

std::ostream& operator<<(std::ostream& os, const SClientHandleSuitesCmd&);

#endif
