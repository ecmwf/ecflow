#ifndef ERROR_CMD_HPP_
#define ERROR_CMD_HPP_
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

class ErrorCmd : public ServerToClientCmd {
public:
   explicit ErrorCmd(const std::string& errorMsg);
	ErrorCmd() : ServerToClientCmd() {}

	void init( const std::string& errorMsg);
	virtual std::ostream& print(std::ostream& os) const;
	virtual bool equals(ServerToClientCmd*) const;
  	virtual bool handle_server_response( ServerReply&, Cmd_ptr cts_cmd, bool debug ) const;

 	virtual std::string error() const { return error_msg_;}   /// Used by test
	virtual bool ok() const { return false; }                 /// Used by group command
   virtual void cleanup() { std::string().swap(error_msg_);} /// run in the server, after command send to client

private:
 	std::string error_msg_;

	friend class boost::serialization::access;
	template<class Archive>
	void serialize( Archive & ar, const unsigned int /*version*/ ) {
 		ar & boost::serialization::base_object< ServerToClientCmd >( *this );
 		ar & error_msg_;
  	}
};

std::ostream& operator<<(std::ostream& os, const ErrorCmd&);

#endif
