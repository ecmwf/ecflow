/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ServerHandler.hpp"

#include <algorithm>
#include <iostream>

#include <QDateTime>
#include <QDebug>
#include <QMessageBox>
#include <QMetaType>
#include <boost/asio/ip/host_name.hpp>

#include "ChangeNotify.hpp"
#include "ConnectState.hpp"
#include "DirectoryHandler.hpp"
#include "MainWindow.hpp"
#include "NodeObserver.hpp"
#include "ServerComObserver.hpp"
#include "ServerComQueue.hpp"
#include "ServerDefsAccess.hpp"
#include "ServerObserver.hpp"
#include "SessionHandler.hpp"
#include "ShellCommand.hpp"
#include "SuiteFilter.hpp"
#include "UIDebug.hpp"
#include "UiLogS.hpp"
#include "UpdateTimer.hpp"
#include "UserMessage.hpp"
#include "VNode.hpp"
#include "VSettings.hpp"
#include "VTaskObserver.hpp"
#include "ecflow/client/ClientInvoker.hpp"
#include "ecflow/core/File.hpp"
#include "ecflow/core/Str.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Node.hpp"
#include "ecflow/node/NodeFwd.hpp"

std::vector<ServerHandler*> ServerHandler::servers_;
std::string ServerHandler::localHostName_;

// #define __UI_SERVEROBSERVER_DEBUG
// #define __UI_SERVERCOMOBSERVER_DEBUG
#define __UI_SERVERUPDATE_DEBUG

ServerHandler::ServerHandler(const std::string& name,
                             const std::string& host,
                             const std::string& port,
                             const std::string& user,
                             bool ssl,
                             bool http)
    : name_(name),
      host_(host),
      port_(port),
      user_(user),
      ssl_(ssl),
      http_(http),
      client_(nullptr),
      updating_(false),
      communicating_(false),
      vRoot_(nullptr),
      suiteFilter_(new SuiteFilter()),
      comQueue_(nullptr),
      activity_(NoActivity),
      compatibility_(CanBeCompatible), // not yet known!
      connectState_(new ConnectState()),
      prevServerState_(SState::RUNNING),
      conf_(nullptr) {
    if (localHostName_.empty()) {
        localHostName_ = boost::asio::ip::host_name();
    }

    // Create longname
    longName_     = host_ + "@" + port_;
    fullLongName_ = name_ + "[" + longName_ + "]";

    conf_ = new VServerSettings(this);

    // Connect up the timer for refreshing the server info. The timer has not
    // started yet.
    refreshTimer_ = new UpdateTimer(this);
    connect(refreshTimer_, SIGNAL(timeout()), this, SLOT(refreshServerInfo()));

    // We will need to pass various non-Qt types via signals and slots for error messages.
    // So we need to register these types.
    if (servers_.empty()) {
        qRegisterMetaType<std::string>("std::string");
        qRegisterMetaType<QList<ecf::Aspect::Type>>("QList<ecf::Aspect::Type>");
        qRegisterMetaType<std::vector<ecf::Aspect::Type>>("std::vector<ecf::Aspect::Type>");
        // to do this we need to incluce Node.hpp !
        qRegisterMetaType<const Node*>("const Node*");
    }

    // Add this instance to the servers_ list.
    servers_.push_back(this);

    createClient(true);
}

// the real deletion is performed by logout()/queueLoggedOut()
ServerHandler::~ServerHandler() {
    Q_ASSERT(comQueue_ == nullptr);
    Q_ASSERT(client_ == nullptr);
}

void ServerHandler::logoutAndDelete() {
    activity_ = DeleteActivity;

    stopRefreshTimer();

    // Save settings
    saveConf();

    // Notify the observers
    broadcast(&ServerObserver::notifyServerDelete);

    // The queue must be deleted before the client, since the thread might
    // be running a job on the client!!
    bool queueLoggedOut = true;
    if (comQueue_) {
        queueLoggedOut = comQueue_->logout();
    }

    // ComQueue will delete itself - it will delete the client and the ComThread as well
    comQueue_ = nullptr;
    client_   = nullptr;

    // Remove itself from the server vector
    auto it = std::find(servers_.begin(), servers_.end(), this);
    if (it != servers_.end())
        servers_.erase(it);

    delete vRoot_;
    delete connectState_;
    delete suiteFilter_;
    delete conf_;

    if (queueLoggedOut) {
        deleteLater();
    }
}

void ServerHandler::queueLoggedOut() {
    // at this point both the queue and the thread are being deleted
    assert(client_ == nullptr);
    assert(comQueue_ == nullptr);
    assert(activity_ == DeleteActivity || activity_ == ClientRecreateActivity);
    if (activity_ == DeleteActivity) {
        deleteLater();
    }
    else if (activity_ == ClientRecreateActivity) {
        createClient(false);
    }
}

// called from the constructor with init=true
// must be called from/after recreateClient() with init=false
void ServerHandler::createClient(bool init) {
    assert(client_ == nullptr);
    assert(comQueue_ == nullptr);

    // Create the client invoker. At this point it is empty.
    try {
        // True passed in to avoid reading SSL from the environment
        client_ = new ClientInvoker(true /*gui*/, host_, port_);
    }
    catch (std::exception& e) {
        std::string errMsg = "Could not create ClientInvoker for host=" + host_ + " port=" + port_ + " !";

        if (e.what())
            errMsg += " " + std::string(e.what());

        UiLog().err() << errMsg;
        client_ = nullptr;
        failedClientServer(errMsg);
    }

    bool ssl_enabled = false;
    std::string ssl_error;
    bool http_enabled = false;
    std::string http_error;

    if (client_) {
#ifdef ECF_OPENSSL
        if (ssl_) {
            try {
                client_->enable_ssl();
                ssl_enabled = true;
            }
            catch (std::exception& e) {
                ssl_error = std::string(e.what());
            }
            // this works when there is a problem with the certificate.
            // The server can be still SSL-compatible if a certificate is there.
            // We will detect this situation dutin the initial load!!!
        }
        else {
            client_->disable_ssl();
        }
#endif
        if (http_) {
            try {
                client_->enable_http();
                http_enabled = true;
            }
            catch (std::exception& e) {
                http_error = std::string(e.what());
            }
        }

        if (!init || !user_.empty()) {
            try {
                client_->set_user_name(user_);
            }
            catch (std::exception& e) {
                UiLog().err() << std::string(e.what());
            }
        }

        client_->set_auto_sync(true); // will call sync_local() after each command!!!
        client_->set_retry_connection_period(1);
        client_->set_connection_attempts(1);
        client_->set_throw_on_error(true);
    }

    // Create the vnode root. This will represent the node tree in the viewer, but
    // at this point it is empty.
    if (init) {
        Q_ASSERT(vRoot_ == nullptr);
        vRoot_ = new VServer(this);
    }

    // NOTE: we may not always want to create a thread here because of resource
    // issues; another strategy would be to create threads on demand, only
    // when server communication is about to start.

    // Create the queue for the tasks to be sent to the client (via the ServerComThread)! It will
    // create and take ownership of the ServerComThread. At this point the queue has not started yet.
    comQueue_ = new ServerComQueue(this, client_);

    if (client_) {
        // Load settings
        if (init) {
            loadConf();
        }

        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        // At this point nothing is running or active!!!!

        // we need to set it because in disconnected state reset is ignored!!!
        if (!init) {
            connectState_->state(ConnectState::Undef);
        }

#ifdef ECF_OPENSSL
        if (ssl_ && !ssl_enabled) {
            sslCertificateError(ssl_error);
            return;
        }
#endif

        // Check if the server is compatible with the client. If it is fine
        // reset() will be called to connect to the server and load the defs etc.
        // This is a safety check to avoid communicating with incompatible
        // servers!!!!
        checkServerVersion();
    }
}

void ServerHandler::recreateClient() {
    activity_ = ClientRecreateActivity;

    stopRefreshTimer();

    if (client_) {
        // Save settings
        saveConf();

        // We clear and stop everything before we delete the objects!!
        clearTree();

        // Create an object to inform the observers about the change
        VServerChange change;
        // Notify the observers that the scan has started
        broadcast(&ServerObserver::notifyBeginServerScan, change);
        // Notify the observers that scan has ended
        broadcast(&ServerObserver::notifyEndServerScan);

        connectState_->state(ConnectState::Disconnected);
        connectState_->errorMessage("");
        // setActivity(NoActivity);

        broadcast(&ServerObserver::notifyServerConnectState);
    }

    // The queue must be deleted before the client, since the thread might
    // be running a job on the client!!
    bool queueLoggedOut = true;
    if (comQueue_) {
        queueLoggedOut = comQueue_->logout();
    }

    // ComQueue will delete itself - it will delete the client and the ComThread as well
    comQueue_ = nullptr;
    client_   = nullptr;

    if (queueLoggedOut)
        createClient(false);
}

void ServerHandler::setSsl(bool ssl) {
    if (ssl != ssl_) {
        ssl_ = ssl;

        if (connectState_->state() != ConnectState::VersionIncompatible &&
            connectState_->state() != ConnectState::FailedClient) {
            recreateClient();
        }
    }
}

void ServerHandler::setHttp(bool http) {
    if (http != http_) {
        http_ = http;

        if (connectState_->state() != ConnectState::VersionIncompatible &&
            connectState_->state() != ConnectState::FailedClient) {
            recreateClient();
        }
    }
}

void ServerHandler::setUser(const std::string& user) {
    if (user != user_) {
        user_ = user;

        if (client_ && connectState_->state() != ConnectState::VersionIncompatible &&
            connectState_->state() != ConnectState::FailedClient) {
            recreateClient();
        }
    }
}

bool ServerHandler::isDisabled() const {
    return (connectState_->state() == ConnectState::Disconnected ||
            connectState_->state() == ConnectState::VersionIncompatible ||
            connectState_->state() == ConnectState::SslIncompatible ||
            connectState_->state() == ConnectState::SslCertificateError || compatibility_ == Incompatible);
}

bool ServerHandler::isInLogout() const {
    return (activity() == DeleteActivity || activity() == ClientRecreateActivity);
}

//-----------------------------------------------
// Refresh/update
//-----------------------------------------------

bool ServerHandler::updateInfo(int& basePeriod, int& currentPeriod, int& drift, int& toNext) {
    if (!refreshTimer_->isActive())
        return false;

    toNext        = secsTillNextRefresh();
    basePeriod    = conf_->intValue(VServerSettings::UpdateRate);
    currentPeriod = refreshTimer_->interval() / 1000;
    drift         = -1;
    if (conf_->boolValue(VServerSettings::AdaptiveUpdate)) {
        drift = conf_->intValue(VServerSettings::AdaptiveUpdateIncrement);
    }

    return true;
}

int ServerHandler::currentRefreshPeriod() const {
    if (!refreshTimer_->isActive())
        return -1;

    return refreshTimer_->interval() / 1000;
}

int ServerHandler::secsSinceLastRefresh() const {
    return static_cast<int>(lastRefresh_.secsTo(QDateTime::currentDateTime()));
}

int ServerHandler::secsTillNextRefresh() const {
    if (refreshTimer_->isActive())
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
        return refreshTimer_->remainingTime() / 1000;
#else
        return 0;
#endif
    return -1;
}

void ServerHandler::stopRefreshTimer() {
    refreshTimer_->stop();
#ifdef __UI_SERVERUPDATE_DEBUG
    UiLogS(this).dbg() << "ServerHandler::stopRefreshTimer -->";
#endif
    broadcast(&ServerComObserver::notifyRefreshTimerStopped);
}

void ServerHandler::startRefreshTimer() {
    UiLogS(this).dbg() << "ServerHandler::startRefreshTimer -->";

    if (!conf_->boolValue(VServerSettings::AutoUpdate)) {
        return;
    }

    // If we are not connected to the server the
    // timer should not run.
    if (isDisabled())
        return;

    if (!refreshTimer_->isActive()) {
        int rate = conf_->intValue(VServerSettings::UpdateRate);
        if (rate <= 0)
            rate = 1;
        refreshTimer_->setInterval(rate * 1000);
        refreshTimer_->start();
        broadcast(&ServerComObserver::notifyRefreshTimerStarted);
    }

#ifdef __UI_SERVERUPDATE_DEBUG
    UiLogS(this).dbg() << " refreshTimer interval: " << refreshTimer_->interval();
#endif
}

void ServerHandler::updateRefreshTimer() {
    UiLogS(this).dbg() << "ServerHandler::updateRefreshTimer -->";

    if (isInLogout()) {
        stopRefreshTimer();
        return;
    }

    if (!conf_->boolValue(VServerSettings::AutoUpdate)) {
        stopRefreshTimer();
        return;
    }

    // If we are not connected to the server the
    // timer should not run.
    if (isDisabled())
        return;

    int rate = conf_->intValue(VServerSettings::UpdateRate);
    if (rate <= 0)
        rate = 1;

    if (refreshTimer_->isActive()) {
        refreshTimer_->stop();
    }

    refreshTimer_->setInterval(rate * 1000);
    refreshTimer_->start();
    broadcast(&ServerComObserver::notifyRefreshTimerChanged);

#ifdef __UI_SERVERUPDATE_DEBUG
    UiLogS(this).dbg() << " refreshTimer interval: " << refreshTimer_->interval();
#endif
}

void ServerHandler::driftRefreshTimer() {
    if (isInLogout()) {
        stopRefreshTimer();
        return;
    }

    if (!conf_->boolValue(VServerSettings::AutoUpdate)) {
        return;
    }

    // We increase the update frequency
    if (activity_ != LoadActivity && conf_->boolValue(VServerSettings::AdaptiveUpdate)) {
#ifdef __UI_SERVERUPDATE_DEBUG
        UiLogS(this).dbg() << "driftRefreshTimer -->";
#endif

        int rate      = conf_->intValue(VServerSettings::UpdateRate);
        int baseDelta = conf_->intValue(VServerSettings::AdaptiveUpdateIncrement);
        int delta     = baseDelta; // linear mode

        // doubling mode
        if (conf_->stringValue(VServerSettings::AdaptiveUpdateMode) == "doubling") {
            delta = refreshTimer_->interval() / 1000 - rate;
            if (delta == 0)
                delta = baseDelta;
        }
        // x modes
        else {
            float f = conf_->stringValue(VServerSettings::AdaptiveUpdateMode).toFloat();
            if (f >= 1. && f <= 5.) {
                delta = (refreshTimer_->interval() / 1000 - rate) * (f - 1);
                if (delta == 0)
                    delta = 1;
            }
        }

        refreshTimer_->drift(delta, conf_->intValue(VServerSettings::MaxAdaptiveUpdateRate));

        broadcast(&ServerComObserver::notifyRefreshTimerChanged);
    }

#ifdef __UI_SERVERUPDATE_DEBUG
    UiLogS(this).dbg() << "driftRefreshTimer interval: " << refreshTimer_->interval();
#endif
}

// returns true if the current total (drifted) period is within the maximum allowed
bool ServerHandler::checkRefreshTimerDrift() const {
    if (isInLogout()) {
        return true;
    }

    if (!conf_->boolValue(VServerSettings::AutoUpdate)) {
        return true;
    }

    if (activity_ != LoadActivity && conf_->boolValue(VServerSettings::AdaptiveUpdate)) {
        return (refreshTimer_->interval() * 1000 < conf_->intValue(VServerSettings::MaxAdaptiveUpdateRate) * 60);
    }
    return true;
}

// mark that a refresh request was sent to the queue
void ServerHandler::refreshScheduled() {
    lastRefresh_ = QDateTime::currentDateTime();
    broadcast(&ServerComObserver::notifyRefreshScheduled);
}

// mark that a refresh request was sent to the queue
void ServerHandler::refreshFinished() {
    broadcast(&ServerComObserver::notifyRefreshFinished);
}

void ServerHandler::setActivity(Activity ac) {
    activity_ = ac;
    broadcast(&ServerObserver::notifyServerActivityChanged);
}

ServerHandler* ServerHandler::addServer(const std::string& name,
                                        const std::string& host,
                                        const std::string& port,
                                        const std::string& user,
                                        bool ssl,
                                        bool http) {
    auto* sh = new ServerHandler(name, host, port, user, ssl, http);
    return sh;
}

void ServerHandler::removeServer(ServerHandler* server) {
    auto it = std::find(servers_.begin(), servers_.end(), server);
    if (it != servers_.end()) {
        ServerHandler* s = *it;
        servers_.erase(it);
        s->logoutAndDelete();
    }
}

ServerHandler* ServerHandler::findServer(const std::string& alias) {
    for (auto it = servers_.begin(); it != servers_.end(); ++it) {
        ServerHandler* s = *it;

        if (s->name() == alias) {
            return s;
        }
    }
    return nullptr; // did not find it
}

// This function can be called many times so we need to avoid locking the mutex.
SState::State ServerHandler::serverState() {
    if (connectState_->state() != ConnectState::Normal || activity() == LoadActivity) {
        prevServerState_ = SState::RUNNING;
    }
    // If we are here we can be sure that the defs pointer is not being deleted!! We
    // access it without locking the mutex!!!
    else {
        // If get the defs can be 100% sure that it is not being deleted!! So we can
        // access it without locking the mutex!!!
        defs_ptr d = safelyAccessSimpleDefsMembers();
        if (d && d.get()) {
            prevServerState_ = d->set_server().get_state();
            return prevServerState_;
        }
    }

    return prevServerState_;
}

// This function can be called many times so we need to avoid locking the mutex.
NState::State ServerHandler::state(bool& suspended) {
    if (connectState_->state() != ConnectState::Normal || activity() == LoadActivity)
        return NState::UNKNOWN;

    suspended = false;

    defs_ptr d = safelyAccessSimpleDefsMembers();
    if (d && d.get()) {
        suspended = d->isSuspended();
        return d->state();
    }

    return NState::UNKNOWN;
}

defs_ptr ServerHandler::defs() {
    defs_ptr null;

    if (client_) {
        return client_->defs();
    }
    else {
        return null;
    }
}

defs_ptr ServerHandler::safelyAccessSimpleDefsMembers() {
    defs_ptr null;

    // The defs might be deleted during reset so it cannot be accessed.
    if (activity_ == LoadActivity) {
        return null;
    }
    // Otherwise it is safe to access certain non-vector members
    else if (client_) {
        return client_->defs();
    }
    else {
        return null;
    }
}

//-------------------------------------------------------------
// Run client tasks.
//
// The preferred way to run client tasks is to define and add a task to the queue. The
// queue will manage the task and will send it to the ClientInvoker. When the task
// finishes the ServerHandler::clientTaskFinished method is called where the
// result/reply can be processed.
//--------------------------------------------------------------

// It is protected! Practically it means we
// we can only run it through CommandHandler!!!
void ServerHandler::runCommand(const std::vector<std::string>& cmd) {
    if (isDisabled())
        return;

    // We do not know if the server is compatible
    if (compatibility_ == CanBeCompatible) {
        // aync - will call reset if successfull
        checkServerVersion();
        return;
    }

    // Shell command - we should not reach this point
    if (cmd.size() > 0 && cmd[0] == "sh") {
        UI_ASSERT(0, "cmd=" << cmd[0]);
        return;
    }

    VTask_ptr task = VTask::create(VTask::CommandTask);
    task->command(cmd);
    comQueue_->addTask(task);
}

// It is protected! Practically it means we
// we can only run it through CommandHandler!!!
void ServerHandler::runCommand(const std::string& cmd) {
    if (isDisabled())
        return;

    // We do not know if the server is compatible
    if (compatibility_ == CanBeCompatible) {
        // aync - will call reset if successfull
        checkServerVersion();
        return;
    }

    // Shell command - we should not reach this point
    if (cmd.size() >= 2 && cmd.substr(0, 2) == "sh") {
        return;
    }

    VTask_ptr task = VTask::create(VTask::CommandTask);
    task->commandAsStr(cmd);
    comQueue_->addTask(task);
}

void ServerHandler::run(VTask_ptr task) {
    if (isDisabled())
        return;

    // We do not know if the server is compatible
    if (compatibility_ == CanBeCompatible) {
        // aync - will call reset if successfull
        checkServerVersion();
        return;
    }

    switch (task->type()) {
        case VTask::ScriptTask:
            return script(task);
            break;
        case VTask::JobTask:
            return job(task);
            break;
        case VTask::OutputTask:
            return jobout(task);
            break;
        case VTask::ManualTask:
            return manual(task);
            break;
        case VTask::JobStatusFileTask:
            return jobstatus(task);
            break;
        case VTask::HistoryTask:
        case VTask::MessageTask:
        case VTask::StatsTask:
        case VTask::ScriptPreprocTask:
        case VTask::ScriptEditTask:
        case VTask::ScriptSubmitTask:
        case VTask::SuiteListTask:
        case VTask::WhySyncTask:
        case VTask::ZombieListTask:
        case VTask::ZombieCommandTask:
        case VTask::JobStatusTask:
        case VTask::PlugTask:
            comQueue_->addTask(task);
            break;
        default:
            // If we are here we have an unhandled task type.
            task->status(VTask::REJECTED);
            break;
    }
}

void ServerHandler::script(VTask_ptr task) {
    /*static std::string errText="no script!\n"
                            "check ECF_FILES or ECF_HOME directories, for read access\n"
                            "check for file presence and read access below files directory\n"
                            "or this may be a 'dummy' task.\n";*/

    task->param("clientPar", "script");
    comQueue_->addTask(task);
}

void ServerHandler::job(VTask_ptr task) {
    /*static std::string errText="no script!\n"
                            "check ECF_FILES or ECF_HOME directories, for read access\n"
                            "check for file presence and read access below files directory\n"
                            "or this may be a 'dummy' task.\n";*/

    task->param("clientPar", "job");
    comQueue_->addTask(task);
}

void ServerHandler::jobout(VTask_ptr task) {
    // static std::string errText="no job output...";

    task->param("clientPar", "jobout");
    comQueue_->addTask(task);
}

void ServerHandler::jobstatus(VTask_ptr task) {
    // static std::string errText="no job output...";

    task->param("clientPar", "stat");
    comQueue_->addTask(task);
}

void ServerHandler::manual(VTask_ptr task) {
    // std::string errText="no manual ...";
    task->param("clientPar", "manual");
    comQueue_->addTask(task);
}

// The core refresh function. That should be the only one directly accessing the queue.
void ServerHandler::refreshInternal() {
    // We add and refresh task to the queue. On startup this function can be called
    // before the comQueue_ was created so we need to check if it exists.
    if (comQueue_) {
        comQueue_->addNewsTask();
    }
}

// The user initiated a refresh - using the toolbar/menu buttons or
// called after a user command was issued
void ServerHandler::refresh() {
    if (isInLogout()) {
        stopRefreshTimer();
        return;
    }

    if (isDisabled())
        return;

    // We do not know if the server is compatible
    if (compatibility_ == CanBeCompatible) {
        // aync - will call reset if successfull
        checkServerVersion();
        return;
    }

    refreshInternal();

    // Reset the timer to its original value (i.e. remove the drift)
    updateRefreshTimer();
}

// This slot is called by the timer regularly to get news from the server.
void ServerHandler::refreshServerInfo() {
    if (isInLogout()) {
        stopRefreshTimer();
        return;
    }

    UiLogS(this).dbg() << "Auto refreshing server";
    refreshInternal();

    // We reduce the update frequency
    driftRefreshTimer();
}

//======================================================================================
// Manages node changes.
//======================================================================================

// This slot is called when a node changes.
void ServerHandler::slotNodeChanged(const Node* nc, std::vector<ecf::Aspect::Type> aspect) {
    UiLogS(this).dbg() << "ServerHandler::slotNodeChanged - node: " + nc->name();

    // for(std::vector<ecf::Aspect::Type>::const_iterator it=aspect.begin(); it != aspect.end(); ++it)
    //     UserMessage::message(UserMessage::DBG, false, std::string(" aspect: ") + ecf::convert_to<std::string>(*it));

    // This can happen if we initiated a reset while we sync in the thread
    if (vRoot_->isEmpty()) {
        UiLogS(this).dbg() << " tree is empty - no change applied!";
        return;
    }

    VNode* vn = vRoot_->toVNode(nc);

    // We must have this VNode
    assert(vn != nullptr);

    // Begin update for the VNode
    VNodeChange change;
    vRoot_->beginUpdate(vn, aspect, change);

    // TODO: what about the infopanel or breadcrumbs??????
    if (change.ignore_) {
        UiLogS(this).dbg() << " update ignored";
        return;
    }
    else {
        // Notify the observers
        broadcast(&NodeObserver::notifyBeginNodeChange, vn, aspect, change);

        // End update for the VNode
        vRoot_->endUpdate(vn, aspect, change);

        // Notify the observers
        broadcast(&NodeObserver::notifyEndNodeChange, vn, aspect, change);

        UiLogS(this).dbg() << " update applied";
    }
}

void ServerHandler::addNodeObserver(NodeObserver* obs) {
    auto it = std::find(nodeObservers_.begin(), nodeObservers_.end(), obs);
    if (it == nodeObservers_.end()) {
        nodeObservers_.push_back(obs);
    }
}

void ServerHandler::removeNodeObserver(NodeObserver* obs) {
    auto it = std::find(nodeObservers_.begin(), nodeObservers_.end(), obs);
    if (it != nodeObservers_.end()) {
        nodeObservers_.erase(it);
    }
}

void ServerHandler::broadcast(NoMethod proc, const VNode* node) {
    for (auto it = nodeObservers_.begin(); it != nodeObservers_.end(); ++it)
        ((*it)->*proc)(node);
}

void ServerHandler::broadcast(NoMethodV1 proc,
                              const VNode* node,
                              const std::vector<ecf::Aspect::Type>& aspect,
                              const VNodeChange& change) {
    // When the observers are being notified (in a loop) they might
    // want to remove themselves from the observer list. This will cause a crash. To avoid
    // this we create a copy of the observers and use it in the notification loop.
    std::vector<NodeObserver*> nObsCopy = nodeObservers_;

    for (auto it = nObsCopy.begin(); it != nObsCopy.end(); ++it) {
        ((*it)->*proc)(node, aspect, change);
    }

#if 0
    for(std::vector<NodeObserver*>::const_iterator it=nodeObservers_.begin(); it != nodeObservers_.end(); ++it)
		((*it)->*proc)(node,aspect,change);
#endif
}

//---------------------------------------------------------------------------
// Manages Defs changes and desf observers. Defs observers are notified when
// there is a change.
//---------------------------------------------------------------------------

// This slot is called when the Defs change.
void ServerHandler::slotDefsChanged(std::vector<ecf::Aspect::Type> aspect) {
    UiLog().dbg() << "ServerHandler::slotDefsChanged -->";
    // for(std::vector<ecf::Aspect::Type>::const_iterator it=aspect.begin(); it != aspect.end(); ++it)
    //     UserMessage::message(UserMessage::DBG, false, std::string(" aspect: ") + ecf::convert_to<std::string>(*it));

    // Begin update for the VNode
    // VNodeChange change;
    vRoot_->beginUpdate(aspect);

    // Notify the observers
    // broadcast(&NodeObserver::notifyBeginNodeChange,vn,aspect,change);

    // End update for the VNode
    // vRoot_->endUpdate(vn,aspect,change);

    // Notify the observers
    // broadcast(&NodeObserver::notifyEndNodeChange,vn,aspect,change);

    // UserMessage::message(UserMessage::DBG, false," --> Update applied");

    for (auto it = serverObservers_.begin(); it != serverObservers_.end(); ++it)
        (*it)->notifyDefsChanged(this, aspect);
}

void ServerHandler::addServerObserver(ServerObserver* obs) {
    auto it = std::find(serverObservers_.begin(), serverObservers_.end(), obs);
    if (it == serverObservers_.end()) {
        serverObservers_.push_back(obs);
#ifdef __UI_SERVEROBSERVER_DEBUG
        UiLog(this).dbg() << "ServerHandler::addServerObserver -->  " << obs;
#endif
    }
}

void ServerHandler::removeServerObserver(ServerObserver* obs) {
#ifdef __UI_SERVEROBSERVER_DEBUG
    UI_FUNCTION_LOG_S(this)
#endif
    auto it = std::find(serverObservers_.begin(), serverObservers_.end(), obs);
    if (it != serverObservers_.end()) {
        serverObservers_.erase(it);
#ifdef __UI_SERVEROBSERVER_DEBUG
        UiLog(this).dbg() << " remove: " << obs;
#endif
    }
}

void ServerHandler::broadcast(SoMethod proc) {
#ifdef __UI_SERVEROBSERVER_DEBUG
    UI_FUNCTION_LOG_S(this)
#endif

    bool checkExistence = true;

    // When the observers are being notified (in a loop) they might
    // want to remove themselves from the observer list. This will cause a crash. To avoid
    // this we create a copy of the observers and use it in the notification loop.
    std::vector<ServerObserver*> sObsCopy = serverObservers_;

    for (auto it = sObsCopy.begin(); it != sObsCopy.end(); ++it) {
        // We need to check if the given observer is still in the original list. When we delete the server, due to
        // dependencies it is possible that the observer is already deleted at this point.
        if (!checkExistence ||
            std::find(serverObservers_.begin(), serverObservers_.end(), *it) != serverObservers_.end()) {
            ((*it)->*proc)(this);
        }
    }
}

void ServerHandler::broadcast(SoMethodV1 proc, const VServerChange& ch) {
    for (auto it = serverObservers_.begin(); it != serverObservers_.end(); ++it)
        ((*it)->*proc)(this, ch);
}

void ServerHandler::broadcast(SoMethodV2 proc, const std::string& strVal) {
    for (auto it = serverObservers_.begin(); it != serverObservers_.end(); ++it)
        ((*it)->*proc)(this, strVal);
}

//------------------------------------------------
// ServerComObserver
//------------------------------------------------

void ServerHandler::addServerComObserver(ServerComObserver* obs) {
    auto it = std::find(serverComObservers_.begin(), serverComObservers_.end(), obs);
    if (it == serverComObservers_.end()) {
        serverComObservers_.push_back(obs);
#ifdef __UI_SERVERCOMOBSERVER_DEBUG
        UiLog(this).dbg() << "ServerHandler::addServerComObserver -->  " << obs;
#endif
    }
}

void ServerHandler::removeServerComObserver(ServerComObserver* obs) {
    auto it = std::find(serverComObservers_.begin(), serverComObservers_.end(), obs);
    if (it != serverComObservers_.end()) {
        serverComObservers_.erase(it);
#ifdef __UI_SERVERCOMOBSERVER_DEBUG
        UiLog(this).dbg() << "ServerHandler::removeServerComObserver --> " << obs;
#endif
    }
}

void ServerHandler::broadcast(SocMethod proc) {
    bool checkExistence = true;

    // When the observers are being notified (in a loop) they might
    // want to remove themselves from the observer list. This will cause a crash. To avoid
    // this we create a copy of the observers and use it in the notification loop.
    std::vector<ServerComObserver*> sObsCopy = serverComObservers_;

    for (auto it = sObsCopy.begin(); it != sObsCopy.end(); ++it) {
        // We need to check if the given observer is still in the original list. When we delete the server, due to
        // dependencies it is possible that the observer is already deleted at this point.
        if (!checkExistence ||
            std::find(serverComObservers_.begin(), serverComObservers_.end(), *it) != serverComObservers_.end())
            ((*it)->*proc)(this);
    }
}

//-------------------------------------------------------------------
// This slot is called when the comThread finished the given task!!
//-------------------------------------------------------------------

// There was a drastic change during the SYNC! As a safety measure we need to clear
// the tree. We will rebuild it when the SYNC finishes.
void ServerHandler::slotRescanNeed() {
    clearTree();
}

void ServerHandler::clientTaskFinished(VTask_ptr task, const ServerReply& serverReply) {
    UiLogS(this).dbg() << "ServerHandler::clientTaskFinished -->";

    // The status of the tasks sent from the info panel must be properly set to
    // FINISHED!! Only that will notify the observers about the change!!

    // task finished but a reset is already on its way
    if (activity() == LoadActivity && task->type() != VTask::ResetTask) {
        // TODO: more refinement here is needed
        return;
    }

    // The news reply has to be carefully checked
    if (task->type() == VTask::NewsTask) {
        // this was a news() called to check the ssl-incompatibilty. If it works we
        // can suppos everyting is fine and can continue with the init!!!
        if (task->param("sslcheck") == "1") {
            compatibleServer();
            return;
        }

        // we've just asked the server if anything has changed - has it?
        refreshFinished();

        switch (serverReply.get_news()) {
            case ServerReply::NO_NEWS: {
                // no news, nothing to do
                UiLogS(this).dbg() << " NO_NEWS";

                // If we just regained the connection we need to reset
                if (connectionGained()) {
                    reset();
                }
                break;
            }

            case ServerReply::NEWS: {
                // yes, something's changed - synchronise with the server

                // If we just regained the connection we need to reset
                UiLogS(this).dbg() << " NEWS - send SYNC command";
                if (connectionGained()) {
                    reset();
                }
                else {
                    comQueue_->addSyncTask();
                }
                break;
            }

            case ServerReply::DO_FULL_SYNC: {
                // yes, a lot of things have changed - we need to reset!!!!!!
                UiLogS(this).dbg() << " DO_FULL_SYNC - will reset";
                connectionGained();
                reset();
                break;
            }

            default: {
                break;
            }
        }
    }

    // reset is also special
    else if (task->type() == VTask::ResetTask) {
        // If not yet connected but the sync task was successful
        resetFinished();
    }

    // the very first command sent to a server
    else if (task->type() == VTask::ServerVersionTask) {
        QString version  = QString::fromStdString(serverReply.get_string());
        int majorVersion = 0;
        if (!version.isEmpty()) {
            QStringList versionLst = version.split(".");
            if (versionLst.count() > 0)
                majorVersion = versionLst[0].toInt();
        }
        if (majorVersion >= 5) {
            compatibleServer();
        }
        else {
            incompatibleServer(version.toStdString());
        }
    }

    // the rest of the tasks
    else {
        UiLogS(this).dbg() << " " << task->typeString() << " finished";

        // Potentially all the comands can call sync_local() implicitly so
        // we need to check for all of them if a sync happened

        // This typically happens when a suite is added/removed
        if (serverReply.full_sync()) {
            UiLogS(this).dbg() << " full sync requested - rescanTree";

            // we do not want a new suite list through rescanTree() if the task itself was
            // to get a new suite list!!
            // with this we try to avoid infinite recursion!!!
            bool neednewSuiteList = task->type() != VTask::SuiteListTask;

            // This might update the suites + restart the timer
            rescanTree(neednewSuiteList);
        }
        else if (serverReply.in_sync()) {
            // The tree can be empty for various reasons, we might have cleared it during a sync. In
            // this case we need to rebuild it.
            // We only rebuild the tree when have a proper connection
            if (vRoot_->isEmpty() && connectState_->state() == ConnectState::Normal) {
                // we do not want a new suite list through rescanTree() unless the task itself
                // was a sync!
                // With this we try to avoid unnecessary calls to update the suite list and avoid infinite recursion!!!
                bool neednewSuiteList = task->type() != VTask::SyncTask;

                // This might update the suites + restart the timer
                rescanTree(neednewSuiteList);
            }
            broadcast(&ServerObserver::notifyEndServerSync);
        }

        // See which type of task finished. What we do now will depend on that.
        switch (task->type()) {

            // CommandTask silently performs sync_local().
            case VTask::CommandTask: {
                updateRefreshTimer();
                break;
            }

            // WhySyncTask performs sync_local(true).
            case VTask::WhySyncTask: {
                updateRefreshTimer();
                break;
            }

            case VTask::ScriptTask:
            case VTask::ManualTask:
            case VTask::HistoryTask:
            case VTask::JobTask:
            case VTask::JobStatusFileTask: {
                task->reply()->fileReadMode(VReply::ServerReadMode);
                task->reply()->setText(serverReply.get_string());
                task->reply()->setReadTruncatedTo(truncatedLinesFromServer(task->reply()->text()));
                break;
            }

            case VTask::OutputTask: {
                task->reply()->fileReadMode(VReply::ServerReadMode);

                std::string err;
                VFile_ptr f(VFile::create(false));
                f->setFetchMode(VFile::ServerFetchMode);
                f->setFetchModeStr("fetched from server " + name());
                f->setSourcePath(task->reply()->fileName());
                f->setTruncatedTo(truncatedLinesFromServer(serverReply.get_string()));
                f->write(serverReply.get_string(), err);
                task->reply()->tmpFile(f);
                break;
            }

            case VTask::MessageTask: {
                task->reply()->setTextVec(serverReply.get_string_vec());
                break;
            }

            // PlugTask silently performs sync_local().
            case VTask::PlugTask: {
                auto target = task->param("target_server_name");
                if (ServerHandler* targetSvr = ServerHandler::find(target)) {
                    targetSvr->refresh();
                }
                break;
            }
            case VTask::StatsTask: {
                task->reply()->text(serverReply.get_string());
                break;
            }

            case VTask::ScriptPreprocTask:
            case VTask::ScriptEditTask: {
                task->reply()->text(serverReply.get_string());
                break;
            }

            // Submitting the task was successful
            case VTask::ScriptSubmitTask: {
                task->reply()->text(serverReply.get_string());
                break;
            }

            case VTask::SuiteListTask: {
                // Update the suite filter with the list of suites actually loaded onto the server.
                // If the suitefilter is enabled this might have only a subset of it in our tree
                updateSuiteFilterWithLoaded(serverReply.get_string_vec());
                break;
            }

            case VTask::ZombieListTask: {
                task->reply()->zombies(serverReply.zombies());
                break;
            }

            default:
                break;
        }

        task->status(VTask::FINISHED);
    }
}

//-------------------------------------------------------------------
// This slot is called when the comThread failed the given task!!
//-------------------------------------------------------------------

void ServerHandler::clientTaskFailed(VTask_ptr task, const std::string& errMsg, const ServerReply& serverReply) {
    UiLogS(this).dbg() << "ServerHandler::clientTaskFailed -->";

    // TODO: suite filter  + ci_ observers

    // task failed but a reset is already on its way
    if (activity() == LoadActivity && task->type() != VTask::ResetTask) {
        // TODO: more refinement here is needed
        return;
    }

    if (task->type() == VTask::NewsTask) {
        // This was a news() called after ServerVersionTask failed!
        if (task->param("sslcheck") == "1" && errMsg.find("possibly non-ssl server") != std::string::npos) {
            sslIncompatibleServer(errMsg);
        }
        else {
            connectionLost(errMsg);
            refreshFinished();
        }
    }

    else if (task->type() == VTask::ResetTask) {
        resetFailed(errMsg);
    }

    // the very first command sent to a server
    else if (task->type() == VTask::ServerVersionTask) {
        if (serverReply.invalid_argument()) {
            incompatibleServer("");
        }
        // if there is no EOF it could be an SSL incomptability issue!
        else if (!serverReply.eof()) {
            // but ... the error message is not make it 100% certain!
            // So we will ask for the news! It will give back an error message
            // that be could properly parsed to identify if it is really
            // an SSL issue!
            compatibility_ = Compatibility::CanBeCompatible;
            comQueue_->addNewsTask("sslcheck", "1");
        }
        else {
            connectionLost(errMsg);
        }
    }
    else if (task->type() == VTask::CommandTask) {
        // comQueue_->addSyncTask();
        task->reply()->setErrorText(errMsg);
        task->status(VTask::ABORTED);
        UserMessage::message(UserMessage::WARN, true, errMsg);
    }
    else if (task->type() == VTask::OutputTask) {
        task->reply()->appendErrorText(errMsg);
        task->status(VTask::ABORTED);
    }
    else if (task->type() == VTask::PlugTask) {
        task->reply()->setErrorText(errMsg);
        task->status(VTask::ABORTED);
        UserMessage::message(UserMessage::WARN, true, errMsg);
    }
    else {
        if (errMsg.empty())
            connectionLost(errMsg);

        task->reply()->setErrorText(errMsg);
        task->status(VTask::ABORTED);
    }
}

void ServerHandler::connectionLost(const std::string& errMsg) {
    connectState_->state(ConnectState::Lost);
    connectState_->errorMessage(errMsg);
    broadcast(&ServerObserver::notifyServerConnectState);
}

bool ServerHandler::connectionGained() {
    if (connectState_->state() != ConnectState::Normal) {
        connectState_->state(ConnectState::Normal);
        broadcast(&ServerObserver::notifyServerConnectState);
        return true;
    }
    return false;
}

void ServerHandler::disconnectServer() {
    if (connectState_->state() != ConnectState::Disconnected) {
        connectState_->state(ConnectState::Disconnected);
        broadcast(&ServerObserver::notifyServerConnectState);

        // Stop the queue
        comQueue_->disable();

        // Stop the timer
        stopRefreshTimer();
    }
}

void ServerHandler::connectServer() {
    if (connectState_->state() == ConnectState::Disconnected) {
        // Start the queue
        comQueue_->enable();

        // Start the timer
        startRefreshTimer();

        // Try to get the news
        refreshInternal();

        // TODO: attach the observers!!!!
    }
}

void ServerHandler::checkServerVersion() {
    comQueue_->addServerVersionTask();
}

void ServerHandler::failedClientServer(const std::string& msg) {
    compatibility_ = Incompatible;
    stopRefreshTimer();
    if (comQueue_) {
        comQueue_->disable();
    }

    connectState_->state(ConnectState::FailedClient);
    connectState_->errorMessage(msg);
    broadcast(&ServerObserver::notifyServerConnectState);
}

void ServerHandler::compatibleServer() {
    compatibility_ = Compatible;
    reset();
}

void ServerHandler::incompatibleServer(const std::string& version) {
    compatibility_ = Incompatible;
    stopRefreshTimer();
    comQueue_->disable();
    setActivity(NoActivity);

    connectState_->state(ConnectState::VersionIncompatible);
    connectState_->errorMessage("Server version = " + ((version.empty()) ? "UNKNOWN" : version) +
                                " !  Client can only communicate to servers with version >= 5.0.0! ");
    broadcast(&ServerObserver::notifyServerConnectState);
}

void ServerHandler::sslIncompatibleServer(const std::string& msg) {
    compatibility_ = Incompatible;
    stopRefreshTimer();
    comQueue_->disable();

    connectState_->state(ConnectState::SslIncompatible);
    std::string errStr = "Cannot communicate to server.";
#if ECF_OPENSSL
    if (ssl_) {
        errStr += " Server is marked as SSL in the UI. Please check if the server is really SSL-enabled! Also check "
                  "settings in Manage servers dialogue!";
    }
    else {
        errStr += " Server is marked as non-SSL in the UI. Please check if SSL is really disabled in the server! Also "
                  "check settings in Manage servers dialogue!";
    }
#else
    errStr += " ecFlow was built without SSL support but it migth be enabled in the server!";
#endif

    connectState_->errorMessage(errStr + "\n\n" + msg);
    broadcast(&ServerObserver::notifyServerConnectState);
}

void ServerHandler::sslCertificateError(const std::string& msg) {
#if ECF_OPENSSL
    assert(ssl_);
    compatibility_ = Incompatible;
    stopRefreshTimer();
    comQueue_->disable();

    connectState_->state(ConnectState::SslCertificateError);
    std::string errStr =
        "Cannot communicate to server. Server is marked as SSL in the UI, but an SSL certificate error was detected.";

    connectState_->errorMessage(errStr + "\n\n" + msg);
    broadcast(&ServerObserver::notifyServerConnectState);
#endif
}

void ServerHandler::reset() {
    UiLogS(this).dbg() << "ServerHandler::reset -->";

    if (connectState_->state() == ConnectState::Disconnected ||
        connectState_->state() == ConnectState::VersionIncompatible ||
        connectState_->state() == ConnectState::SslIncompatible ||
        connectState_->state() == ConnectState::SslCertificateError ||
        connectState_->state() == ConnectState::FailedClient || compatibility_ == Incompatible) {
        return;
    }

    //---------------------------------
    // First part of reset: clearing
    //---------------------------------

    if (comQueue_->prepareReset()) {
        // Stop the timer
        stopRefreshTimer();

        // First part of reset: clear the tree
        clearTree();

        // Second part of reset: loading

        // Indicate that we reload the defs
        setActivity(LoadActivity);

        // mark the current moment as last refresh
        lastRefresh_ = QDateTime::currentDateTime();

        // NOTE: at this point the queue is not running but reset() will start it.
        // While the queue is in reset mode it does not accept tasks.
        comQueue_->reset();
    }
    else {
        UiLogS(this).dbg() << " skip reset";
    }
}

// The reset was successful
void ServerHandler::resetFinished() {
    setActivity(NoActivity);

    // Set server host and port in defs. It is used to find the server of
    // a given node in the viewer.
    {
        ServerDefsAccess defsAccess(this); // will reliquish its resources on destruction

        defs_ptr defs = defsAccess.defs();
        if (defs != nullptr) {
            ServerState& st = defs->set_server();
            st.hostPort(std::make_pair(host_, port_));
        }
    }

    // Create an object to inform the observers about the change
    VServerChange change;

    // Begin the full scan to get the tree. This call does not actually
    // run the scan but counts how many suits will be available.
    vRoot_->beginScan(change);

    // Notify the observers that the scan has started
    broadcast(&ServerObserver::notifyBeginServerScan, change);

    // Finish full scan
    vRoot_->endScan();

    // assert(change.suiteNum_ == vRoot_->numOfChildren());

    // Notify the observers that scan has ended
    broadcast(&ServerObserver::notifyEndServerScan);

    // The suites might have been changed
    updateSuiteFilter(true);

    // Restart the timer
    startRefreshTimer();

    // Set the connection state
    if (connectState_->state() != ConnectState::Normal) {
        connectState_->state(ConnectState::Normal);
        broadcast(&ServerObserver::notifyServerConnectState);
    }
}

// The reset failed and we could not connect to the server, e.g. because the the server
// may be down, or there is a network error, or the authorisation is missing.
void ServerHandler::resetFailed(const std::string& errMsg) {
    // This status is indicated by the connectStat_. Each object needs to be aware of it
    // and do its tasks accordingly.

    // Create an object to inform the observers about the change
    VServerChange change;
    // Notify the observers that the scan has started
    broadcast(&ServerObserver::notifyBeginServerScan, change);
    // Notify the observers that scan has ended
    broadcast(&ServerObserver::notifyEndServerScan);

    connectState_->state(ConnectState::Lost);
    connectState_->errorMessage(errMsg);
    setActivity(NoActivity);

    broadcast(&ServerObserver::notifyServerConnectState);

    // Restart the timer
    startRefreshTimer();
}

// This function must be called during a SYNC!!!!!!!!
// If it calls me must be sure that notifyEndServerScan is called
// in the end!!!

void ServerHandler::clearTree() {
    UiLogS(this).dbg() << "ServerHandler::clearTree -->";

    if (!vRoot_->isEmpty()) {
        // Notify observers that the clear is about to begin
        broadcast(&ServerObserver::notifyBeginServerClear);

        // Clear vnode
        vRoot_->clear();

        // Notify observers that the clear ended
        broadcast(&ServerObserver::notifyEndServerClear);
    }

    UiLogS(this).dbg() << " <-- clearTree";
}

void ServerHandler::rescanTree(bool needNewSuiteList) {
    UiLogS(this).dbg() << "ServerHandler::rescanTree -->";

    setActivity(RescanActivity);

    //---------------------------------
    // First part of rescan: clearing
    //---------------------------------

    // Stop the timer
    stopRefreshTimer();

    // Stop the queue as a safety measure: we do not want any changes during the rescan
    comQueue_->suspend(false);

    // clear the tree
    clearTree();

    // At this point nothing is running and the tree is empty (it only contains
    // the root node)

    //--------------------------------------
    // Second part of rescan: loading
    //--------------------------------------

    // Create an object to inform the observers about the change
    VServerChange change;

    // Begin the full scan to get the tree. This call does not actually
    // run the scan but counts how many suits will be available.
    vRoot_->beginScan(change);

    // Notify the observers that the scan has started
    broadcast(&ServerObserver::notifyBeginServerScan, change);

    // Finish full scan
    vRoot_->endScan();

    // Notify the observers that scan has ended
    broadcast(&ServerObserver::notifyEndServerScan);

    // Restart the queue
    comQueue_->start();

    // The suites might have been changed
    updateSuiteFilter(needNewSuiteList);

    // Start the timer
    startRefreshTimer();

    setActivity(NoActivity);

    UiLogS(this).dbg() << "<-- rescanTree";
}

//====================================================
// Suite filter
//====================================================

void ServerHandler::setSuiteFilterWithOne(VNode* n) {
    if (n) {
        if (VNode* sn = n->suite())
            if (VSuiteNode* suite = sn->isSuite()) {
                if (suiteFilter_->isEnabled() == false) {
                    suiteFilter_->setEnabled(true);
                    suiteFilter_->selectOnlyOne(suite->strName());
                    // after reset the loaded suites need to be read again from the server!
                    suiteFilter_->setLoadedInitialised(false);
                    reset();
                }
                else if (suiteFilter_->isOnlyOneFiltered(suite->strName()) == false) {
                    suiteFilter_->selectOnlyOne(suite->strName());
                    // after reset the loaded suites need to be read again from the server!
                    suiteFilter_->setLoadedInitialised(false);
                    reset();
                }
            }
    }
}

void ServerHandler::updateSuiteFilter(SuiteFilter* sf) {
    UI_FUNCTION_LOG_S(this)

    if (isDisabled())
        return;

    // We do not know if the server is compatible
    if (compatibility_ == CanBeCompatible) {
        // aync - will call reset if successfull
        checkServerVersion();
        return;
    }

    if (suiteFilter_->update(sf)) {
        // If only this flag has changed we exec a custom task for it
        if (suiteFilter_->changeFlags().sameAs(SuiteFilter::AutoAddChanged)) {
            comQueue_->addSuiteAutoRegisterTask();
        }
        // Otherwise we need a full reset
        else {
            reset();
        }

        // Save the settings
        conf_->saveSettings();
    }
}

// Update the suite filter with the list of suites actually loaded onto the server.
// If the suitefilter is enabled we might have only a subset of it in our tree.
void ServerHandler::updateSuiteFilterWithLoaded(const std::vector<std::string>& loadedSuites) {
    UI_FUNCTION_LOG_S(this)
    if (suiteFilter_->setLoaded(loadedSuites)) {
        // If the filtered state changed and the filter is active we need to reset
        if (suiteFilter_->isEnabled())
            reset();
    }
}

// Update the suite filter with the list of suites stored in the defs (in the tree). It only
// makes sense if the filter is disabled since in this case the defs stores all the loaded servers.
void ServerHandler::updateSuiteFilterWithDefs() {
    UI_FUNCTION_LOG_S(this)

    if (suiteFilter_->isEnabled())
        return;

    // The filte ris disabled
    std::vector<std::string> defSuites;
    vRoot_->suites(defSuites);
    suiteFilter_->setLoaded(defSuites);
}

// Only called internally after reset or serverscan!!
void ServerHandler::updateSuiteFilter(bool needNewSuiteList) {
    UI_FUNCTION_LOG_S(this)

    if (isDisabled())
        return;

    // We do not know if the server is compatible
    if (compatibility_ == CanBeCompatible) {
        // aync - will call reset if successfull
        checkServerVersion();
        return;
    }

    // We only fetch the full list of loaded suites from the server
    // via the thread when
    //   -the suiteFilter is enabled!
    if (suiteFilter_->isEnabled()) {
        // This will call updateSuiteFilterWithLoaded()
        if (needNewSuiteList) {
            comQueue_->addSuiteListTask();
        }
    }
    // Otherwise the defs contains the full list of suites
    else {
        std::vector<std::string> defSuites;
        vRoot_->suites(defSuites);
        suiteFilter_->setLoaded(defSuites);
    }

#if 0
    //We only fetch the full list of loaded suites from the server
    //via the thread when
    //  -the suiteFilter is not yet initialised
    //   OR
    //  -the suiteFilter is observerved and it is enabled!
    if(!suiteFilter_->isLoadedInitialised() ||
       (suiteFilter_->hasObserver() && suiteFilter_->isEnabled()))
	{
		//This will call updateSuiteFilterWithLoaded()
		comQueue_->addSuiteListTask();
	}
	else
	{
		std::vector<std::string> defSuites;
		vRoot_->suites(defSuites);
		suiteFilter_->setLoaded(defSuites);
	}
#endif
}

bool ServerHandler::readFromDisk() const {
    return conf_->boolValue(VServerSettings::ReadFromDisk);
}

QString ServerHandler::uidForServerLogTransfer() const {
    return conf_->stringValue(VServerSettings::UidForServerLogTransfer);
}

int ServerHandler::maxSizeForTimelineData() const {
    return conf_->intValue(VServerSettings::MaxSizeForTimelineData);
}

QString ServerHandler::nodeMenuMode() const {
    return conf_->stringValue(VServerSettings::NodeMenuMode);
}

QString ServerHandler::defStatusNodeMenuMode() const {
    return conf_->stringValue(VServerSettings::NodeMenuModeForDefStatus);
}

void ServerHandler::rename(const std::string& name) {
    auto oldName  = name_;
    name_         = name;
    fullLongName_ = name_ + "[" + longName_ + "]";
    broadcast(&ServerObserver::notifyServerRenamed, oldName);
}

void ServerHandler::confChanged(VServerSettings::Param par, VProperty* /*prop*/) {
    switch (par) {
        case VServerSettings::AutoUpdate:
        case VServerSettings::UpdateRate:
        case VServerSettings::AdaptiveUpdate:
        case VServerSettings::AdaptiveUpdateMode:
        case VServerSettings::AdaptiveUpdateIncrement:
            updateRefreshTimer();
            break;
        case VServerSettings::MaxAdaptiveUpdateRate: {
            if (!checkRefreshTimerDrift()) {
                updateRefreshTimer();
            }
            break;
        }
        case VServerSettings::NodeMenuMode:
            MainWindow::updateMenuMode(this);
            break;

        case VServerSettings::NotifyAbortedEnabled:
        case VServerSettings::NotifyRestartedEnabled:
        case VServerSettings::NotifyLateEnabled:
        case VServerSettings::NotifyZombieEnabled:
        case VServerSettings::NotifyAliasEnabled:
            checkNotificationState(par);
            break;
        default:
            break;
    }
}

void ServerHandler::checkNotificationState(VServerSettings::Param par) {
    std::string id = VServerSettings::notificationId(par);
    if (id.empty())
        return;

    bool enabled = false;
    for (std::vector<ServerHandler*>::const_iterator it = servers_.begin(); it != servers_.end(); ++it) {
        ServerHandler* s = *it;
        if (s->conf()->boolValue(par)) {
            enabled = true;
            break;
        }
    }

    ChangeNotify::updateNotificationStateFromServer(id, enabled);
}

// Called from changeNotify
bool ServerHandler::checkNotificationState(const std::string& notifierId) {
    bool enabled               = false;
    VServerSettings::Param par = VServerSettings::notificationParam(notifierId);

    for (auto it = servers_.begin(); it != servers_.end(); ++it) {
        ServerHandler* s = *it;
        if (s->conf()->boolValue(par)) {
            enabled = true;
            break;
        }
    }

    return enabled;
}

void ServerHandler::saveSettings() {
    for (auto it = servers_.begin(); it != servers_.end(); ++it)
        (*it)->saveConf();
}

void ServerHandler::saveConf() {
    conf_->saveSettings();
}

void ServerHandler::loadConf() {
    // This will call confChanged for any non-default settings
    conf_->loadSettings();
}

void ServerHandler::writeDefs(const std::string& fileName) {
    if (isDisabled() || compatibility_ == CanBeCompatible)
        return;

    comQueue_->suspend(true);
    ServerDefsAccess defsAccess(this); // will reliquish its resources on destruction
    defs_ptr defs = defsAccess.defs();
    if (defs) {
        defs->save_as_filename(fileName, PrintStyle::MIGRATE);
    }
    comQueue_->start();
}

void ServerHandler::writeDefs(VInfo_ptr info, const std::string& fileName) {
    if (isDisabled() || compatibility_ == CanBeCompatible)
        return;

    if (!info || !info->node())
        return;

    comQueue_->suspend(true);
    ServerDefsAccess defsAccess(this); // will reliquish its resources on destruction
    defs_ptr defs = defsAccess.defs();
    if (defs) {
        PrintStyle style(PrintStyle::MIGRATE);
        std::ofstream out(fileName.c_str());
        out << "defs_state MIGRATE" << std::endl;
        out << info->node()->node()->print();
        out << std::endl;
        out.close();
    }
    comQueue_->start();
}

//--------------------------------------------
// Other
//--------------------------------------------

void ServerHandler::searchBegan() {
    if (isDisabled() || compatibility_ == CanBeCompatible)
        return;

    UiLogS(this).dbg() << "ServerHandler::searchBegan --> suspend queue";
    comQueue_->suspend(true);
}

void ServerHandler::searchFinished() {
    if (isDisabled() || compatibility_ == CanBeCompatible)
        return;

    UiLogS(this).dbg() << "ServerHandler::searchFinished --> start queue";
    comQueue_->start();
}

int ServerHandler::truncatedLinesFromServer(const std::string& txt) const {
    // if the text is truncated the following line is added to the bottom of it:
    // # >>>>>>>> File truncated down to 15. Truncated from the end of file <<<<<<<<<
    // We search for this string and if truncation did happen we indicate it in the reply
    size_t txtSize = txt.size();
    if (txt.find(">> File truncated down to", (txtSize > 200) ? (txtSize - 100) : 0) != std::string::npos) {
        return conf_->intValue(VServerSettings::MaxOutputFileLines);
    }

    return -1;
}

//--------------------------------------------------------------
//
//   Find the server for a node.
//   TODO: this is just a backup method. We might not want to use it
//         at all, since it is not safe.
//
//--------------------------------------------------------------

ServerHandler* ServerHandler::find(const std::string& name) {
    for (auto it = servers_.begin(); it != servers_.end(); ++it)
        if ((*it)->name() == name)
            return *it;
    return nullptr;
}
