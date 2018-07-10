#ifndef DEFS_CMD_HPP_
#define DEFS_CMD_HPP_
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #14 $ 
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
#include "DefsCache.hpp"
class AbstractServer;

//================================================================================
// Paired with CtsNodeCmd(GET)
// Client---CtsNodeCmd(GET)---->Server-----(DefsCmd | SNodeCmd)--->client:
//================================================================================
class DefsCmd : public ServerToClientCmd {
public:
  	DefsCmd(AbstractServer* as, bool save_edit_history = false);
	DefsCmd(){}

	void init(AbstractServer* as, bool save_edit_history);

  	virtual bool hasDefs() const { return true; }
  	virtual bool handle_server_response( ServerReply&, Cmd_ptr cts_cmd, bool debug ) const;
  	virtual std::ostream& print(std::ostream& os) const;
	virtual bool equals(ServerToClientCmd*) const;
   virtual void cleanup() { std::string().swap(full_server_defs_as_string_); } /// run in the server, after command send to client

private:

 	std::string full_server_defs_as_string_;

   friend class cereal::access;
   template<class Archive>
   void serialize(Archive & ar, std::uint32_t const version )
   {
 	   ar & cereal::base_class< ServerToClientCmd >( this );

 	   if (Archive::is_saving::value) {
 	      // Avoid copying the string. As this could be very large
 	      ar & DefsCache::full_server_defs_as_string_;
 	   }
 	   else {
 	      ar & full_server_defs_as_string_;
 	   }
 	}
};

std::ostream& operator<<(std::ostream& os, const DefsCmd&);

#endif
