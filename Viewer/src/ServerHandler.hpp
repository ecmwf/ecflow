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

#include <QThread>

#include "Defs.hpp"

#include "ViewNodeInfo.hpp"


class ClientInvoker;

// -------------------------------------------------------
// ServerComThread - a class to handler communication with
// an ecflow server.
// -------------------------------------------------------

class ServerComThread : public QThread
{
	Q_OBJECT

public:
	enum ComType {COMMAND, NEWS, SYNC};

	ServerComThread();

	void sendCommand(ClientInvoker *ci, ServerComThread::ComType comType);
	void setCommandString(const std::vector<std::string> command);
	ComType commandType();
	void stop();

protected:
	void run();

private:
	ClientInvoker *ci_;
	std::vector<std::string> command_;
	ComType comType_;
};



class ServerHandler : public QObject // ingerits from QObject in order to gain signal/slots
{
	Q_OBJECT

public:
		ServerHandler(const std::string& name,const std::string&  port);
		~ServerHandler();

		const std::string name() const {return name_;}
		const std::string longName() const {return longName_;}
		const std::string& port() const {return port_;}
		defs_ptr defs() const;
		int suiteNum() const;
		Node* suiteAt(int) const;
		int indexOfSuite(Node* node);

		int numberOfNodes();
		Node* findNode(int globalIndex);
		Node* findNode(int globalIndex,int& total,Node *parent);

		bool communicating() {return communicating_;}

		int update();
		void setUpdatingStatus(bool newStatus) {updating_ = newStatus;}

		static const std::vector<ServerHandler*>& servers() {return servers_;}
		static ServerHandler* addServer(const std::string &server, const std::string &port);
		static int numOfImmediateChildren(Node*);
		static Node* immediateChildAt(Node *parent,int pos);
		static int indexOfImmediateChild(Node *node);
		static void command(std::vector<ViewNodeInfo_ptr>,std::string);
		static ServerHandler* find(const std::string& longName);
		static ServerHandler* find(const std::pair<std::string,std::string>& hostPort);
		static ServerHandler* find(Node *node);
		static void addServerCommand(const std::string &name, const std::string command);
		static std::string resolveServerCommand(const std::string &name);
		static void updateAll();

protected:

		void setCommunicatingStatus(bool c) {communicating_ = c;}

		std::string name_;
		std::string port_;
		ClientInvoker* client_;
		std::string longName_;
		bool updating_;
		bool communicating_;

		static std::vector<ServerHandler*> servers_;
		static std::map<std::string, std::string> commands_;

private slots:

		void commandSent();  // invoked when a command has finished being sent to the server

private:

		ServerComThread *comThread();

		ServerComThread *comThread_;
};


#endif
