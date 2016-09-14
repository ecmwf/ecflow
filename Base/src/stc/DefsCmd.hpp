#ifndef DEFS_CMD_HPP_
#define DEFS_CMD_HPP_
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #14 $ 
//
// Copyright 2009-2016 ECMWF. 
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description :
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8

#include "ServerToClientCmd.hpp"
#include "ServerToClientCmdContext.hpp"
class AbstractServer;

//================================================================================
// Paired with CtsNodeCmd(GET)
// Client---CtsNodeCmd(GET)---->Server-----(DefsCmd | SNodeCmd)--->client:
//================================================================================
class DefsCmd : public ServerToClientCmd {
public:
  	DefsCmd(AbstractServer* as, bool save_edit_history = false);
	DefsCmd() {}

	void init(AbstractServer* as, bool save_edit_history);

   defs_ptr defs() const { return defs_; }

  	virtual bool hasDefs() const { return defs_.get() != NULL; }
  	virtual bool handle_server_response( ServerReply&, Cmd_ptr cts_cmd, bool debug ) const;
  	virtual std::ostream& print(std::ostream& os) const;
	virtual bool equals(ServerToClientCmd*) const;

private:
 	defs_ptr defs_;

	friend class boost::serialization::access;
	template<class Archive>
	void serialize( Archive & ar, const unsigned int /*version*/ ) {
	   ecf::ServerToClientCmdContext context;
 		ar & boost::serialization::base_object< ServerToClientCmd >( *this );
      ar & defs_;
  	}
};

std::ostream& operator<<(std::ostream& os, const DefsCmd&);

#endif
