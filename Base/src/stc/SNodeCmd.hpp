#ifndef SNODE_CMD_HPP_
#define SNODE_CMD_HPP_
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #6 $ 
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
// Paired with CtsNodeCmd(Get)
// Client---CtsNodeCmd(GET)---->Server-----(SNodeCmd | SNodeCmd)--->client:
//================================================================================
class SNodeCmd : public ServerToClientCmd {
public:
   SNodeCmd(AbstractServer* as,node_ptr node);
   SNodeCmd() {}

   void init(AbstractServer* as, node_ptr node);

   virtual bool handle_server_response( ServerReply&, Cmd_ptr cts_cmd, bool debug ) const;
   virtual std::ostream& print(std::ostream& os) const;
   virtual bool equals(ServerToClientCmd*) const;
   virtual bool hasNode() const { return true; }             /// used by group command

private:
   node_ptr get_node_ptr(std::string& error_msg) const;
   std::string the_node_str_;

   friend class boost::serialization::access;
   template<class Archive>
   void serialize( Archive & ar, const unsigned int /*version*/ ) {
      ar & boost::serialization::base_object< ServerToClientCmd >( *this );
      ar & the_node_str_;
   }
};

std::ostream& operator<<(std::ostream& os, const SNodeCmd&);

#endif
