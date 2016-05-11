#ifndef SERVER_TO_CLIENT_CMD_HPP_
#define SERVER_TO_CLIENT_CMD_HPP_
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #19 $ 
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

#include <string>
#include <vector>

#include <boost/serialization/base_object.hpp>      // base class serialization
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/assume_abstract.hpp>
#include <boost/serialization/shared_ptr.hpp>

#include "Cmd.hpp"
#include "NodeFwd.hpp"
#include "ServerReply.hpp"
class AbstractServer;

//================================================================================
// Start of Server->client
//================================================================================
class ServerToClientCmd {
public:
	virtual ~ServerToClientCmd();

	virtual std::ostream& print(std::ostream& os) const = 0;
	virtual bool equals(ServerToClientCmd*) const { return true;}

	virtual const std::string& get_string() const;             /// Used by group command, can return any string, including file contents
	virtual bool ok() const { return true;}                    /// Used by group command
   virtual bool hasDefs() const { return false; }             /// used by group command
   virtual bool hasNode() const { return false; }             /// used by group command
	virtual std::string error() const { return std::string();} /// Used by test
	virtual bool isOkCmd() const { return false; }             /// Used by server, to not respond back, client assumes OK

  	// Called in client, if any data to be returned , the set on class ServerReply
	// Cmd_ptr cts_cmd  can used for additional context.
	// return true, if client should exit, returns false, if further work required, ie blocking, errors, etc
  	virtual bool handle_server_response( ServerReply&, Cmd_ptr cts_cmd, bool debug ) const = 0;

protected:
	ServerToClientCmd(){}
private:
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive &ar, const unsigned int /*version*/) {}
};
BOOST_SERIALIZATION_ASSUME_ABSTRACT(ServerToClientCmd)

#endif
