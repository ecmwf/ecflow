/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_viewer_ServerHandler_HPP
#define ecflow_viewer_ServerHandler_HPP

#include <ctime>
#include <deque>
#include <string>
#include <utility>
#include <vector>

#include <QMutex>

#include "VInfo.hpp"
#include "VReply.hpp"
#include "VServerSettings.hpp"
#include "VTask.hpp"
#include "ecflow/base/ServerProtocol.hpp"
#include "ecflow/node/Defs.hpp"

class ClientInvoker;
class ServerReply;

class ConnectState;
class NodeObserver;
class ServerComQueue;
class ServerObserver;
class ServerComObserver;
class SuiteFilter;
class UpdateTimer;
class VNodeChange;
class VServer;
class VServerChange;
class VSettings;
class VSuiteNode;

class ServerHandler : public QObject {
    Q_OBJECT // inherits from QObject in order to gain signal/slots

        friend class ServerDefsAccess;
    friend class ServerComQueue;
    friend class CommandHandler;

public:
    enum Activity { NoActivity, LoadActivity, RescanActivity, DeleteActivity, ClientRecreateActivity };
    enum Compatibility { Compatible, Incompatible, CanBeCompatible };

    const std::string& name() const { return name_; }
    const std::string& host() const { return host_; }
    const std::string& longName() const { return longName_; }
    const std::string& fullLongName() const { return fullLongName_; }
    const std::string& port() const { return port_; }
    bool isSsl() const { return protocol_ == ecf::Protocol::Ssl; }
    bool isHttp() const { return protocol_ == ecf::Protocol::Http; }
    bool isHttps() const { return protocol_ == ecf::Protocol::Https; }
    ecf::Protocol protocol() const { return protocol_; }
    const std::string& user() { return user_; }
    void setProtocol(ecf::Protocol protocol);
    void setUser(const std::string& user);

    Activity activity() const { return activity_; }
    ConnectState* connectState() const { return connectState_; }
    bool communicating() { return communicating_; }
    bool isEnabled() const { return !isDisabled(); }
    bool isDisabled() const;
    bool isInLogout() const;

    bool readFromDisk() const;
    QString uidForServerLogTransfer() const;
    int maxSizeForTimelineData() const;

    SuiteFilter* suiteFilter() const { return suiteFilter_; }
    QString nodeMenuMode() const;
    QString defStatusNodeMenuMode() const;

    void setSuiteFilterWithOne(VNode*);
    void updateSuiteFilter(SuiteFilter*);
    void updateSuiteFilterWithDefs();

    void connectServer();
    void disconnectServer();
    void reset();

    void refresh();
    void setUpdatingStatus(bool newStatus) { updating_ = newStatus; }

    VServer* vRoot() const { return vRoot_; }
    SState::State serverState();
    NState::State state(bool& isSuspended);

    void run(VTask_ptr);

    void addNodeObserver(NodeObserver* obs);
    void removeNodeObserver(NodeObserver* obs);

    void addServerObserver(ServerObserver* obs);
    void removeServerObserver(ServerObserver* obs);

    void addServerComObserver(ServerComObserver* obs);
    void removeServerComObserver(ServerComObserver* obs);

    void confChanged(VServerSettings::Param, VProperty*);
    VServerSettings* conf() const { return conf_; }

    bool isLocalHost() { return (localHostName_ == host_ || host_ == "localhost"); }

    void rename(const std::string& name);

    static void saveSettings();

    static const std::vector<ServerHandler*>& servers() { return servers_; }
    static ServerHandler* addServer(const std::string& name,
                                    const std::string& host,
                                    const std::string& port,
                                    const std::string& user,
                                    ecf::Protocol protocol);
    static void removeServer(ServerHandler*);
    static ServerHandler* findServer(const std::string& alias);

    void searchBegan();
    void searchFinished();
    bool updateInfo(int& basePeriod, int& currentPeriod, int& drift, int& toNext);
    int currentRefreshPeriod() const;
    QDateTime lastRefresh() const { return lastRefresh_; }
    int secsSinceLastRefresh() const;
    int secsTillNextRefresh() const;

    static bool checkNotificationState(const std::string& notifierId);

    static ServerHandler* find(const std::string& name);

    void writeDefs(const std::string& fileName);
    void writeDefs(VInfo_ptr info, const std::string& fileName);

protected:
    ServerHandler(const std::string& name,
                  const std::string& host,
                  const std::string& port,
                  const std::string& user,
                  ecf::Protocol protocol);
    ~ServerHandler() override;
    void logoutAndDelete();
    void queueLoggedOut();

    // We should only be able to run runCommand through CommandHandler!!!
    void runCommand(const std::vector<std::string>& cmd);
    void runCommand(const std::string& cmd);

    void connectToServer();
    void setCommunicatingStatus(bool c) { communicating_ = c; }
    void clientTaskFinished(VTask_ptr task, const ServerReply& serverReply);
    void clientTaskFailed(VTask_ptr task, const std::string& errMsg, const ServerReply& serverReply);

    static void checkNotificationState(VServerSettings::Param par);

    bool checkRefreshTimerDrift() const;
    void refreshScheduled();
    void refreshFinished();

    std::string name_;
    std::string host_;
    std::string port_;
    std::string user_;
    ecf::Protocol protocol_;
    ClientInvoker* client_;
    std::string longName_;
    std::string fullLongName_;
    bool updating_;
    bool communicating_;
    std::vector<NodeObserver*> nodeObservers_;
    std::vector<ServerObserver*> serverObservers_;
    std::vector<ServerComObserver*> serverComObservers_;

    VServer* vRoot_;

    // The list of suites the server makes accessible
    SuiteFilter* suiteFilter_;

    static std::vector<ServerHandler*> servers_;

private Q_SLOTS:
    void refreshServerInfo();
    void slotNodeChanged(const Node* n, std::vector<ecf::Aspect::Type>);
    void slotDefsChanged(std::vector<ecf::Aspect::Type>);
    void slotRescanNeed();

private:
    void createClient(bool init);
    void recreateClient();
    void refreshInternal();
    void resetFinished();
    void resetFailed(const std::string& errMsg);
    void clearTree();
    void rescanTree(bool needNewSuiteList = true);
    void connectionLost(const std::string& errMsg);
    bool connectionGained();
    void checkServerVersion();
    void failedClientServer(const std::string& msg);
    void compatibleServer();
    void incompatibleServer(const std::string& version);
    void sslIncompatibleServer(const std::string& msg);
    void sslCertificateError(const std::string& msg);

    void updateSuiteFilterWithLoaded(const std::vector<std::string>&);
    void updateSuiteFilter(bool needNewSuiteList);

    // Handle the refresh timer
    void stopRefreshTimer();
    void startRefreshTimer();
    void updateRefreshTimer();
    void driftRefreshTimer();

    void script(VTask_ptr req);
    void job(VTask_ptr req);
    void jobout(VTask_ptr req);
    void jobstatus(VTask_ptr req);
    void manual(VTask_ptr req);

    defs_ptr defs();
    defs_ptr safelyAccessSimpleDefsMembers();

    void setActivity(Activity activity);

    typedef void (ServerObserver::*SoMethod)(ServerHandler*);
    typedef void (ServerObserver::*SoMethodV1)(ServerHandler*, const VServerChange&);
    typedef void (ServerObserver::*SoMethodV2)(ServerHandler*, const std::string&);
    void broadcast(SoMethod);
    void broadcast(SoMethodV1, const VServerChange&);
    void broadcast(SoMethodV2, const std::string&);

    typedef void (NodeObserver::*NoMethod)(const VNode*);
    typedef void (NodeObserver::*NoMethodV1)(const VNode*, const std::vector<ecf::Aspect::Type>&, const VNodeChange&);
    typedef void (NodeObserver::*NoMethodV2)(const VNode*, const VNodeChange&);
    void broadcast(NoMethod, const VNode*);
    void broadcast(NoMethodV1, const VNode*, const std::vector<ecf::Aspect::Type>&, const VNodeChange&);
    void broadcast(NoMethodV2, const VNode*, const VNodeChange&);

    typedef void (ServerComObserver::*SocMethod)(ServerHandler*);
    void broadcast(SocMethod);

    void saveConf();
    void loadConf();

    int truncatedLinesFromServer(const std::string& txt) const;

    QMutex defsMutex_;
    defs_ptr defs_;

    ServerComQueue* comQueue_;

    // std::string targetNodeNames_;      // used when building up a command in ServerHandler::command
    // std::string targetNodeFullNames_;  // used when building up a command in ServerHandler::command

    UpdateTimer* refreshTimer_;
    QDateTime lastRefresh_;

    Activity activity_;
    Compatibility compatibility_;
    ConnectState* connectState_;
    SState::State prevServerState_;

    VServerSettings* conf_;

    static std::string localHostName_;
};

#endif /* ecflow_viewer_ServerHandler_HPP */
