#ifndef SNEWS_CMD_HPP_
#define SNEWS_CMD_HPP_
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #9 $ 
//
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description : class SNewsCmd:  Used to determine any change in the server
//
// The *client_state_change_no* was passed from the client to the server
// The *client_modify_change_no* was passed from the client to the server
//
// The code here needs to coordinate with SSyncCmd
//
// Paired with CtsCmd(NEWS)
// Client---CtsCmd(NEWS)---->Server-----(SNewsCmd)--->client:
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8

#include "ServerToClientCmd.hpp"

class SNewsCmd : public ServerToClientCmd {
public:
   // The constructor is *called* in the server.
   SNewsCmd(unsigned int client_handle, // a reference to a set of suites used by client
            unsigned int client_state_change_no,
            unsigned int client_modify_change_no,
            AbstractServer* as);
   SNewsCmd() : ServerToClientCmd(), news_(ServerReply::NO_NEWS) {}

   void init(unsigned int client_handle, // a reference to a set of suites used by client
            unsigned int client_state_change_no,
            unsigned int client_modify_change_no,
            AbstractServer* as);

   ServerReply::News_t news() const { return news_;} // used by equals only
   bool get_news() const { return ( news_ != ServerReply::NO_NEWS); }

   virtual std::ostream& print(std::ostream& os) const;
   virtual bool equals(ServerToClientCmd*) const;
   virtual bool handle_server_response( ServerReply& server_reply, Cmd_ptr cts_cmd, bool debug ) const;

private:
   // After ECFLOW-182, ServerReply::NO_DEFS no longer used, however kept, to ensure client/server compatibility
   // ie. for new client(viewer) must process this from old server, which could return NO_DEFS
   ServerReply::News_t news_;

   friend class boost::serialization::access;
   template<class Archive>
   void serialize( Archive & ar, const unsigned int /*version*/ ) {
      ar & boost::serialization::base_object< ServerToClientCmd >( *this );
      ar & news_;
   }
};

std::ostream& operator<<(std::ostream& os, const SNewsCmd&);
#endif
