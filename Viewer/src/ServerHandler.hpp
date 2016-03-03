//============================================================================
// Copyright 2015 ECMWF.
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

#include "VReply.hpp"
#include "VTask.hpp"
#include "VInfo.hpp"
#include "VServerSettings.hpp"

#include <QMutex>

class ClientInvoker;
class ServerReply;

class ConnectState;
class NodeObserver;
class ServerHandler;
class ServerComQueue;
class ServerObserver;
class SuiteFilter;
class UpdateTimer;
class VNodeChange;
class VServer;
class VServerChange;
class VSettings;


class ServerHandler : public QObject
{
	Q_OBJECT   // inherits from QObject in order to gain signal/slots

	friend class ServerDefsAccess;
	friend class ServerComQueue;
  
public:
	enum Activity {NoActivity,LoadActivity,RescanActivity};

	const std::string& name() const {return name_;}
	const std::string& host() const {return host_;}
	const std::string& longName() const {return longName_;}
	const std::string& port() const {return port_;}

	Activity activity() const {return activity_;}
	ConnectState* connectState() const {return connectState_;}
	bool communicating() {return communicating_;}
	bool readFromDisk() const;
	SuiteFilter* suiteFilter() const {return suiteFilter_;}

	void updateSuiteFilter(SuiteFilter*);
	void updateSuiteFilterWithDefs();

	void connectServer();
	void disconnectServer();
	void reset();

	void refresh();
	void setUpdatingStatus(bool newStatus) {updating_ = newStatus;}

	VServer* vRoot() const {return vRoot_;}
	SState::State serverState();
	NState::State state(bool& isSuspended);

	void runCommand(const std::vector<std::string>& cmd);
	void run(VTask_ptr);

	void addNodeObserver(NodeObserver* obs);
	void removeNodeObserver(NodeObserver* obs);

	void addServerObserver(ServerObserver* obs);
	void removeServerObserver(ServerObserver* obs);

	void confChanged(VServerSettings::Param,VProperty*);
	VServerSettings* conf() const {return conf_;}

	bool isLocalHost() {return (localHostName_ == host_ || host_ == "localhost");}

	static void saveSettings();

	static const std::vector<ServerHandler*>& servers() {return servers_;}
	static ServerHandler* addServer(const std::string &name,const std::string &host, const std::string &port);
	static void removeServer(ServerHandler*);

	void command(const std::vector<std::string>& fullPaths, const std::vector<std::string>& cmd);
	static void command(VInfo_ptr,const std::vector<std::string>&);
	static void command(std::vector<VInfo_ptr>,std::string);

	void searchBegan();
	void searchFinished();
    int secsSinceLastRefresh() const;
    int secsTillNextRefresh() const;

	static ServerHandler* find(const std::string& name);

protected:
	ServerHandler(const std::string& name,const std::string& host,const std::string&  port);
	~ServerHandler();

	void connectToServer();
	void setCommunicatingStatus(bool c) {communicating_ = c;}
	void clientTaskFinished(VTask_ptr task,const ServerReply& serverReply);
	void clientTaskFailed(VTask_ptr task,const std::string& errMsg);

	static std::string commandToString(const std::vector<std::string>& cmd);
	static void checkNotificationState(VServerSettings::Param par);

	std::string name_;
	std::string host_;
	std::string port_;
	ClientInvoker* client_;
	std::string longName_;
	bool updating_;
	bool communicating_;
	std::vector<NodeObserver*> nodeObservers_;
	std::vector<ServerObserver*> serverObservers_;

    VServer* vRoot_;

    //The list of suites the server makes accessible
    SuiteFilter* suiteFilter_;

	static std::vector<ServerHandler*> servers_;

private Q_SLOTS:
	void errorMessage(std::string message); // invoked when an error message is received
	void refreshServerInfo();
	void slotNodeChanged(const Node* n, std::vector<ecf::Aspect::Type>);
	void slotDefsChanged(std::vector<ecf::Aspect::Type>);
	void slotRescanNeed();

private:
	//Begin and end the initialisation by connecting to the server and syncing.
    void refreshInternal();
    void resetFinished();
	void resetFailed(const std::string& errMsg);
	void clearTree();
	void rescanTree();
	void connectionLost(const std::string& errMsg);
	bool connectionGained();

	void updateSuiteFilterWithLoaded(const std::vector<std::string>&);
	void updateSuiteFilter();

	//Handle the refresh timer
	void stopRefreshTimer();
	void startRefreshTimer();
    void updateRefreshTimer();
    void driftRefreshTimer();

	void script(VTask_ptr req);
	void job(VTask_ptr req);
	void jobout(VTask_ptr req);
	void manual(VTask_ptr req);

	defs_ptr defs();
	defs_ptr safelyAccessSimpleDefsMembers();

	void setActivity(Activity activity);

	typedef void (ServerObserver::*SoMethod)(ServerHandler*);
	typedef void (ServerObserver::*SoMethodV1)(ServerHandler*,const VServerChange&);
	void broadcast(SoMethod,bool checkExitense=false);
	void broadcast(SoMethodV1,const VServerChange&);

	typedef void (NodeObserver::*NoMethod)(const VNode*);
	typedef void (NodeObserver::*NoMethodV1)(const VNode*,const std::vector<ecf::Aspect::Type>&,const VNodeChange&);
	typedef void (NodeObserver::*NoMethodV2)(const VNode*,const VNodeChange&);
	void broadcast(NoMethod,const VNode*);
	void broadcast(NoMethodV1,const VNode*,const std::vector<ecf::Aspect::Type>&,const VNodeChange&);
	void broadcast(NoMethodV2,const VNode*,const VNodeChange&);

	void saveConf();
	void loadConf();

	QMutex           defsMutex_;
	defs_ptr defs_;

	ServerComQueue* comQueue_;

	//std::string targetNodeNames_;      // used when building up a command in ServerHandler::command
	//std::string targetNodeFullNames_;  // used when building up a command in ServerHandler::command

    UpdateTimer* refreshTimer_;
    QDateTime lastRefresh_;

	Activity activity_;
	ConnectState* connectState_;
	SState::State prevServerState_;

	VServerSettings* conf_;

	static std::string localHostName_;
};
    
#endif
