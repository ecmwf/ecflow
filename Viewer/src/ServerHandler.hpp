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

#include <deque>
#include <utility>
#include <string>
#include <vector>
#include <ctime>

#include "Defs.hpp"
#include "AbstractObserver.hpp"

#include "VReply.hpp"
#include "VTask.hpp"
#include "VInfo.hpp"

#include <QThread>
#include <QMutex>
#include <QTimer>

class ClientInvoker;
class ServerReply;

class ConnectState;
class NodeObserver;
class ServerHandler;
class ServerComQueue;
class ServerComThread;
class ServerObserver;
class VNodeChange;
class VServer;
class VServerChange;

class ServerHandler : public QObject
{
	Q_OBJECT   // inherits from QObject in order to gain signal/slots

	friend class ServerDefsAccess;
	friend class ServerComQueue;
  
public:
	enum Activity {NoActivity,LoadActivity};

	const std::string& name() const {return name_;}
	const std::string& host() const {return host_;}
	const std::string& longName() const {return longName_;}
	const std::string& port() const {return port_;}

	Activity activity() const {return activity_;}
	ConnectState* connectState() const {return connectState_;}
	bool communicating() {return communicating_;}
	bool readFromDisk() const {return readFromDisk_;}

	void connectServer();
	void disconnectServer();
	void reset();

	static void resetFirst();

	int update();
	void setUpdatingStatus(bool newStatus) {updating_ = newStatus;}
	void releaseDefs();

	VServer* vRoot() const {return vRoot_;}
	SState::State serverState();
	NState::State state(bool& isSuspended);

	void runCommand(const std::vector<std::string>& cmd);
	void run(VTask_ptr);

	void addNodeObserver(NodeObserver* obs);
	void removeNodeObserver(NodeObserver* obs);

	void addServerObserver(ServerObserver* obs);
	void removeServerObserver(ServerObserver* obs);

	static const std::vector<ServerHandler*>& servers() {return servers_;}
	static ServerHandler* addServer(const std::string &name,const std::string &host, const std::string &port);
	static void removeServer(ServerHandler*);

	static void command(VInfo_ptr,const std::vector<std::string>&, bool resolve);
	static void command(std::vector<VInfo_ptr>,std::string, bool resolve);

	static ServerHandler* find(const std::string& name);
	static ServerHandler* find(Node *node);

	static void addServerCommand(const std::string &name, const std::string command);
	static std::string resolveServerCommand(const std::string &name);
	static void updateAll();

protected:
	ServerHandler(const std::string& name,const std::string& host,const std::string&  port);
	~ServerHandler();

	void connectToServer();
	void setCommunicatingStatus(bool c) {communicating_ = c;}
	void clientTaskFinished(VTask_ptr task,const ServerReply& serverReply);
	void clientTaskFailed(VTask_ptr task,const std::string& errMsg);

	static std::string commandToString(const std::vector<std::string>& cmd);

	std::string name_;
	std::string host_;
	std::string port_;
	ClientInvoker* client_;
	std::string longName_;
	bool updating_;
	bool communicating_;
	std::vector<NodeObserver*> nodeObservers_;
	std::vector<ServerObserver*> serverObservers_;

	bool readFromDisk_;

    VServer* vRoot_;
        
	static std::vector<ServerHandler*> servers_;
	static std::map<std::string, std::string> commands_;

private Q_SLOTS:
	//void commandSent();  // invoked when a command has finished being sent to the server
	void errorMessage(std::string message); // invoked when an error message is received
	//void queryFinished(VReply_ptr);  // invoked when a reply is received from from the server/thread
	void refreshServerInfo();
	void slotNodeChanged(const Node* n, const std::vector<ecf::Aspect::Type>& a);
	void slotDefsChanged(const std::vector<ecf::Aspect::Type>& a);

private:
	//Begin and end the initialisation by connecting to the server and syncing.
	void load();
	void loadFinished();
	void loadFailed(const std::string& errMsg);
	void connectionLost(const std::string& errMsg);
	void connectionGained();

	//Handle the update timer
	void stopRefreshTimer();
	void resetRefreshTimer();

	void script(VTask_ptr req);
	void job(VTask_ptr req);
	void jobout(VTask_ptr req);
	void manual(VTask_ptr req);

	defs_ptr defs();

	typedef void (ServerObserver::*SoMethod)(ServerHandler*);
	typedef void (ServerObserver::*SoMethodV1)(ServerHandler*,const VServerChange&);
	void broadcast(SoMethod);
	void broadcast(SoMethodV1,const VServerChange&);

	typedef void (NodeObserver::*NoMethod)(const VNode*);
	typedef void (NodeObserver::*NoMethodV1)(const VNode*,const std::vector<ecf::Aspect::Type>&,const VNodeChange&);
	typedef void (NodeObserver::*NoMethodV2)(const VNode*,const VNodeChange&);
	void broadcast(NoMethod,const VNode*);
	void broadcast(NoMethodV1,const VNode*,const std::vector<ecf::Aspect::Type>&,const VNodeChange&);
	void broadcast(NoMethodV2,const VNode*,const VNodeChange&);

	QMutex           defsMutex_;

	ServerComQueue* comQueue_;

	//std::string targetNodeNames_;      // used when building up a command in ServerHandler::command
	//std::string targetNodeFullNames_;  // used when building up a command in ServerHandler::command

	int refreshIntervalInSeconds_;
	QTimer refreshTimer_;

	Activity activity_;
	ConnectState* connectState_;
};

// --------------------------------------------------------------
// ServerComQueue - a class to provide a queueing system for
// sending tasks to the ClientIvoker via the ServerComThread.
// --------------------------------------------------------------

class ServerComQueue : public QObject
{
Q_OBJECT

public:
	ServerComQueue(ServerHandler *server,ClientInvoker* client,ServerComThread* comThread);
	~ServerComQueue();

	void addTask(VTask_ptr);
	void addNewsTask();
	void addSyncTask();
	void start();
	void stop();
	void load();
	bool active() const {return active_;}

protected Q_SLOTS:
	void slotRun();

protected Q_SLOTS:
	void slotTaskFinished();
	void slotTaskFailed(std::string);

protected:
	void endLoad();

	ServerHandler *server_;
	ClientInvoker* client_;
	ServerComThread *comThread_;
	QTimer* timer_;
	std::deque<VTask_ptr> tasks_;
	VTask_ptr current_;
	bool wait_;
	bool active_;
	bool load_;
};

// -------------------------------------------------------
// ServerComThread - a class to handler communication with
// an ecflow server.
// -------------------------------------------------------

class ServerComThread : public QThread, public AbstractObserver
{
	Q_OBJECT

public:
	ServerComThread(ServerHandler *server, ClientInvoker *ci);
	~ServerComThread();

	void task(VTask_ptr);
	void stop();

	//From AbstractObserver
	void update(const Node*, const std::vector<ecf::Aspect::Type>&);
	void update(const Defs*, const std::vector<ecf::Aspect::Type>&);
	void update_delete(const Node*);
	void update_delete(const Defs*);

	bool attached() const {return attached_;}
	void attach();
	void detach();

Q_SIGNALS:
	void nodeChanged(const Node*, const std::vector<ecf::Aspect::Type>&);
	void defsChanged(const std::vector<ecf::Aspect::Type>&);
	void failed(std::string message);

protected:
	void run();

private:
	void attach(Node *node);
	void detach(Node *node);

	ServerHandler *server_;
	ClientInvoker *ci_;
	VTask::Type taskType_;
	std::vector<std::string> command_;
	std::map<std::string,std::string> params_;
	std::vector<std::string> contents_;
	NameValueVec vars_;
	std::string nodePath_;
	bool attached_;
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
