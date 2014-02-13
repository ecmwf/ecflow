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

#include <utility>
#include <string>
#include <vector>
#include "Defs.hpp"

#include "ViewNodeInfo.hpp"

class ClientInvoker;

class ServerHandler
{
public:
		ServerHandler(const std::string& name,const std::string&  port);
		~ServerHandler();

		const std::string name() const {return name_;}
		const std::string longName() const {return longName_;}
		const std::string& port() const {return port_;}
		defs_ptr defs() const;
		int suiteNum() const;
		Node* suiteAt(int) const;
		int update();
		void setUpdatingStatus(bool newStatus) {updating_ = newStatus;}

		static const std::vector<ServerHandler*>& servers() {return servers_;}
		static ServerHandler* addServer(const std::string &server, const std::string &port);
		static int numOfImmediateChildren(Node*);
		static Node* immediateChildAt(Node *parent,int pos);
		static void command(std::vector<ViewNodeInfo_ptr>,std::string);
		static ServerHandler* find(const std::string& longName);
		static ServerHandler* find(const std::pair<std::string,std::string>& hostPort);
		static ServerHandler* find(Node *node);
		static void addServerCommand(const std::string &name, const std::string command);
		static std::string resolveServerCommand(const std::string &name);

protected:
		std::string name_;
		std::string port_;
		ClientInvoker* client_;
		std::string longName_;
		bool updating_;

		static std::vector<ServerHandler*> servers_;
		static std::map<std::string, std::string> commands_;
};


#endif
