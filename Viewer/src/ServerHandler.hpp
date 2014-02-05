//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef SERVERHANDLER_HPP_
#define SERVERHANDLER_HPP_

#include <string>
#include <vector>
#include "Defs.hpp"

class ClientInvoker;

class ServerHandler
{
public:
		ServerHandler(const std::string& name, int port);
		~ServerHandler();

		const std::string name() const {return name_;}
		const std::string longName() const {return longName_;}
		int port() const {return port_;}
		defs_ptr defs();
		int suiteNum() const;

		static const std::vector<ServerHandler*>& servers() {return servers_;}
		static ServerHandler* addServer(const std::string &server, int port);
		static void command(std::vector<ServerHandler*>,std::vector<Node*>,std::string);

protected:
		std::string name_;
		int port_;
		ClientInvoker *client_;
		std::string longName_;

		static std::vector<ServerHandler*> servers_;
};


#endif
