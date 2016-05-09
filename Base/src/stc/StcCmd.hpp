#ifndef STC_CMD_HPP_
#define STC_CMD_HPP_
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #8 $ 
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

// Command that are simple replies to the client.
// Originally we had separate commands. However this lead
// TOC overflow on the AIX. Hence in order to minimise global
// symbols due to use of boost serialisation, will use a single command
class StcCmd : public ServerToClientCmd {
public:
	enum Api { OK,
		       BLOCK_CLIENT_SERVER_HALTED,
		       BLOCK_CLIENT_ON_HOME_SERVER,
		       BLOCK_CLIENT_ZOMBIE
		     };
	StcCmd(Api a) :  api_(a) {}
	StcCmd() :  api_(OK) {}

	void init(Api a) { api_ = a;}
	Api api() const { return api_;}

	virtual std::ostream& print(std::ostream& os) const;
	virtual bool equals(ServerToClientCmd*) const;
   	virtual bool handle_server_response( ServerReply& server_reply, Cmd_ptr cts_cmd, bool debug ) const;

	/// Other legitimate ServerToClientCmd commands also return ok() as true
	/// Hence must still have isOkCmd()
	virtual bool ok() const { return api_ == OK; }      // used by group command
	virtual bool isOkCmd() const { return api_ == OK; } // Used if no reply back from server

private:
	Api api_;

	friend class boost::serialization::access;
	template<class Archive>
	void serialize( Archive & ar, const unsigned int /*version*/ ) {
 		ar & boost::serialization::base_object< ServerToClientCmd >( *this );
 		ar & api_;
   	}
};
std::ostream& operator<<(std::ostream& os, const StcCmd&);
#endif
