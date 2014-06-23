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
#include <QMutex>

#include "Defs.hpp"
#include "AbstractObserver.hpp"

#include "ViewNodeInfo.hpp"


class ClientInvoker;
class ServerHandler;

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

	void sendCommand(ServerHandler *server, ClientInvoker *ci, ServerComThread::ComType comType);
	void setCommandString(const std::vector<std::string> command);
	ComType commandType();
	void stop();

protected:
	void run();

private:
	ServerHandler *server_;
	ClientInvoker *ci_;
	std::vector<std::string> command_;
	ComType comType_;
};



class ServerHandler : public QObject, public AbstractObserver
{
	Q_OBJECT   // ingerits from QObject in order to gain signal/slots

	friend class ServerDefsAccess;

public:
		ServerHandler(const std::string& name,const std::string&  port);
		~ServerHandler();

		const std::string name() const {return name_;}
		const std::string longName() const {return longName_;}
		const std::string& port() const {return port_;}
		int numSuites();
		Node* suiteAt(int);
		int indexOfSuite(Node* node);

		int numberOfNodes();
		Node* findNode(int globalIndex);
		Node* findNode(int globalIndex,int& total,Node *parent);

		bool communicating() {return communicating_;}

		int update();
		void setUpdatingStatus(bool newStatus) {updating_ = newStatus;}
		void releaseDefs();

		const std::vector<std::string>& messages(Node* node);
		bool readFile(Node *n,const std::string& id,
				     std::string& fileName,std::string& txt,std::string& errTxt);

		static const std::vector<ServerHandler*>& servers() {return servers_;}
		static ServerHandler* addServer(const std::string &server, const std::string &port);
		static int numOfImmediateChildren(Node*);
		static Node* immediateChildAt(Node *parent,int pos);
		static int indexOfImmediateChild(Node *node);
		static void command(std::vector<ViewNodeInfo_ptr>,std::string);
		static ServerHandler* find(const std::string& longName);
		static ServerHandler* find(const std::pair<std::string,std::string>& hostPort);
		static ServerHandler* find(const std::string& name, const std::string& port);
		static ServerHandler* find(Node *node);
		static void addServerCommand(const std::string &name, const std::string command);
		static std::string resolveServerCommand(const std::string &name);
		static void updateAll();

		//From AbstractObserver
		void update(const Node*, const std::vector<ecf::Aspect::Type>&) {};
		void update(const Defs*, const std::vector<ecf::Aspect::Type>&);

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

		defs_ptr defs();
		ServerComThread *comThread();

		ServerComThread *comThread_;
		QMutex           defsMutex_;
};


// -------------------------------------------------------------------------
// ServerDefsAccess - a class to manage access to the server definition tree
// - required for multi-threaded access
// -------------------------------------------------------------------------

class ServerDefsAccess
{

public:

	ServerDefsAccess(ServerHandler *server);
	~ServerDefsAccess();

	defs_ptr defs();


private:

	ServerHandler *server_;
};



#endif
