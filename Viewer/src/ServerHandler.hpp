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
#include <QTimer>

#include "Defs.hpp"
#include "AbstractObserver.hpp"

#include "NodeInfoQuery.hpp"
#include "ViewNodeInfo.hpp"

class ClientInvoker;
class ServerHandler;

// -------------------------------------------------------
// ServerComThread - a class to handler communication with
// an ecflow server.
// -------------------------------------------------------

class ServerComThread : public QThread, public AbstractObserver
{
	Q_OBJECT

public:
	enum ComType {COMMAND, NEWS, SYNC, FILE, HISTORY};

	ServerComThread();

	void sendCommand(ServerHandler *server, ClientInvoker *ci, ServerComThread::ComType comType);
	void sendCommand(ServerHandler *server, ClientInvoker *ci, ServerComThread::ComType comType,NodeInfoQuery_ptr);
	void setCommandString(const std::vector<std::string> command);
	ComType commandType();
	void stop();

	//From AbstractObserver
	void update(const Node*, const std::vector<ecf::Aspect::Type>&);
	void update(const Defs*, const std::vector<ecf::Aspect::Type>&)  {};

signals:
	void nodeChanged(const Node*, QList<ecf::Aspect::Type>);
	void errorMessage(std::string message);
	void queryFinished(NodeInfoQuery_ptr);

protected:
	void run();
	void initObserver(ServerHandler* server);

private:
	ServerHandler *server_;
	ClientInvoker *ci_;
	std::vector<std::string> command_;
	ComType comType_;
	NodeInfoQuery_ptr query_;
};



class ServerHandler : public QObject
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

		void resetRefreshTimer();
		void query(NodeInfoQuery_ptr);
		void messages(NodeInfoQuery_ptr req);
		void script(NodeInfoQuery_ptr req);
		void job(NodeInfoQuery_ptr req);
		void jobout(NodeInfoQuery_ptr req);
	    void manual(NodeInfoQuery_ptr req);
	    void file(NodeInfoQuery_ptr req,const std::string& errText);

		const std::vector<std::string>& messages(Node* node);
		bool readFile(Node *n,const std::string& id,
				     std::string& fileName,std::string& txt,std::string& errTxt);
		bool readManual(Node *n,std::string& fileName,std::string& txt,std::string& errTxt);

		static const std::vector<ServerHandler*>& servers() {return servers_;}
		static ServerHandler* addServer(const std::string &server, const std::string &port);
		static int numOfImmediateChildren(Node*);
		static Node* immediateChildAt(Node *parent,int pos);
		static int indexOfImmediateChild(Node *node);
		static void command(std::vector<ViewNodeInfo_ptr>,std::string, bool resolve);
		static ServerHandler* find(const std::string& longName);
		static ServerHandler* find(const std::pair<std::string,std::string>& hostPort);
		static ServerHandler* find(const std::string& name, const std::string& port);
		static ServerHandler* find(Node *node);
		static void addServerCommand(const std::string &name, const std::string command);
		static std::string resolveServerCommand(const std::string &name);
		static void updateAll();

		//From AbstractObserver
		//void update(const Node*, const std::vector<ecf::Aspect::Type>&) {};
		//void update(const Defs*, const std::vector<ecf::Aspect::Type>&);

		void addNodeObserver(QObject* obs);
		void removeNodeObserver(QObject* obs);

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
		void errorMessage(std::string message); // invoked when an error message is received
		void queryFinished(NodeInfoQuery_ptr);  // invoked when a reply is received from from the server/thread
		void refreshServerInfo();


private:

		defs_ptr defs();
		ServerComThread *comThread();

		ServerComThread *comThread_;
		QMutex           defsMutex_;


		std::string targetNodeNames_;      // used when building up a command in ServerHandler::command
		std::string targetNodeFullNames_;  // used when building up a command in ServerHandler::command

		int refreshIntervalInSeconds_;
		QTimer refreshTimer_;

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
