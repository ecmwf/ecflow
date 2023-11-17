//============================================================================
// Copyright 2009- ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "OutputDirProvider.hpp"

#include <memory>

#include <QFile>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

#include "OutputDirClient.hpp"
#include "ServerHandler.hpp"
#include "UiLog.hpp"
#include "VConfig.hpp"
#include "VFileTransfer.hpp"
#include "VNode.hpp"
#include "VReply.hpp"

// #define UI_OUTPUTDIRPROVIDER_DEBUG__
// #define UI_OUTPUTDIRPROVIDER_TASK_DEBUG__

OutputDirFetchTask::OutputDirFetchTask(const std::string& name, FetchQueueOwner* owner)
    : AbstractFetchTask(name, owner) {
}

void OutputDirFetchTask::addTryLog(VReply* r, const std::string& txt) const {
    r->addLogTryEntry("<PATH>" + filePath_ + "</PATH> " + txt);
}

//=================================
//
// OutputDirFetchRemoteTask
//
//=================================

OutputDirFetchLogServerTask::OutputDirFetchLogServerTask(FetchQueueOwner* owner)
    : QObject(nullptr),
      OutputDirFetchTask("DirFetchLogServer", owner) {
}

OutputDirFetchLogServerTask::~OutputDirFetchLogServerTask() {
    if (client_) {
        client_->disconnect(this);
    }
}

void OutputDirFetchLogServerTask::deleteClient() {
    if (client_) {
#ifdef UI_OUTPUTDIRPROVIDER_TASK_DEBUG__
        UI_FN_DBG
#endif
        client_->disconnect(this);
        client_->deleteLater();
        client_ = nullptr;
    }
}

void OutputDirFetchLogServerTask::stop() {
    AbstractFetchTask::clear();
    if (status_ == RunningStatus) {
        deleteClient();
    }
}

void OutputDirFetchLogServerTask::clear() {
    AbstractFetchTask::clear();
    deleteClient();
    if (client_) {
        delete client_;
        client_ = nullptr;
    }
}

// Create an output client (to access the logserver) and ask it to the fetch the
// dir asynchronously. The output client will call clientFinished or
// clientError eventually!!
void OutputDirFetchLogServerTask::run() {
#ifdef UI_OUTPUTDIRPROVIDER_TASK_DEBUG__
    UiLog().dbg() << UI_FN_INFO << "filePath=" << filePath_;
#endif
    std::string host, port;
    assert(node_);

    // First try the user defined logserver, then the system defined one
    bool userLogServerUsed = node_->userLogServer(host, port);
    bool sysLogServerUsed  = false;
    if (!userLogServerUsed) {
        sysLogServerUsed = node_->logServer(host, port);
    }
    Q_ASSERT(!userLogServerUsed || !sysLogServerUsed);

#ifdef UI_OUTPUTDIRPROVIDER_TASK_DEBUG__
    UiLog().dbg() << UI_FN_INFO << "host=" << host << " port=" << port;
#endif

    if (userLogServerUsed || sysLogServerUsed) {
        Q_ASSERT(userLogServerUsed || sysLogServerUsed);
        if (client_ && (client_->host() != host || client_->portStr() != port)) {
            deleteClient();
        }

        if (!client_) {
            client_ = new OutputDirClient(host, port, this);

            connect(client_, SIGNAL(error(QString)), this, SLOT(clientError(QString)));

            connect(client_, SIGNAL(progress(QString, int)), this, SLOT(clientProgress(QString, int)));

            connect(client_, SIGNAL(finished()), this, SLOT(clientFinished()));
        }

        Q_ASSERT(client_);

        // fetch the file asynchronously
        status_ = RunningStatus;
        client_->getDir(filePath_);
        return;
    }

    // If we are here there is no output client defined/available
    deleteClient();

    addTryLog(owner_->theReply(), "fetch from logserver: NOT DEFINED");
    finish();
}

void OutputDirFetchLogServerTask::clientFinished() {
    Q_ASSERT(client_);
    auto reply   = owner_->theReply();
    VDir_ptr dir = client_->result();
    if (dir) {
        dir->setFetchMode(VDir::LogServerFetchMode);
        std::string method = "served by " + client_->longName();
        dir->setFetchModeStr(method);
        dir->setFetchDate(QDateTime::currentDateTime());

        addTryLog(reply, "fetch from logserver=" + client_->longName() + " : OK");
        reply->appendDirectory(dir);
        succeed();
        return;
    }

    addTryLog(reply, "fetch from logserver=" + client_->longName() + " : FAILED");
    fail();
}

void OutputDirFetchLogServerTask::clientProgress(QString, int) {
}

void OutputDirFetchLogServerTask::clientError(QString msg) {
    std::string sDesc;
    auto reply = owner_->theReply();
    if (client_) {
        sDesc = "Failed to fetch from " + client_->longName();
        if (!msg.isEmpty()) {
            sDesc += " error: " + msg.toStdString();
        }
        addTryLog(reply, "fetch file from logserver=" + client_->longName() + " : FAILED");
    }
    else {
        sDesc = "Failed to fetch from logserver";
        if (!msg.isEmpty()) {
            sDesc += ": " + msg.toStdString();
        }
        addTryLog(reply, "fetch file from logserver: FAILED");
    }

    reply->appendErrorText(sDesc);
    fail();
}

//=================================
//
// OutputFileFetchLocalTask
//
//=================================
OutputDirFetchLocalTask::OutputDirFetchLocalTask(FetchQueueOwner* owner) : OutputDirFetchTask("DirFetchLocal", owner) {
}

// try to read the logfile from the disk (if the settings allow it)
void OutputDirFetchLocalTask::run() {
#ifdef UI_OUTPUTDIRPROVIDER_TASK_DEBUG__
    UiLog().dbg() << UI_FN_INFO << "filePath=" << filePath_;
#endif
    VDir_ptr res;

    boost::filesystem::path p(filePath_);

    // Is it a directory?
    boost::system::error_code errorCode;
    if (boost::filesystem::is_directory(p, errorCode)) {
        fail();
        return;
    }

    auto reply = owner_->theReply();
    try {
        if (boost::filesystem::exists(p.parent_path())) {
            std::string dirName = p.parent_path().string();
            // if(info_ && info_->isNode() && info_->node())
            if (node_) {
                std::string nodeName = node_->strName();
                std::string pattern  = nodeName + ".";
                res                  = std::make_shared<VDir>(dirName, pattern);
                res->setFetchDate(QDateTime::currentDateTime());
                res->setFetchMode(VDir::LocalFetchMode);
                res->setFetchModeStr("from disk");
                addTryLog(reply, "read from disk: OK");
                reply->appendDirectory(res);
                succeed();
                return;
            }
        }
        addTryLog(reply, "read from disk: NO ACCESS");
        reply->appendErrorText("No access to path on disk!");
    }
    catch (const boost::filesystem::filesystem_error& e) {
        addTryLog(reply, "read from disk: NO ACCESS");
        reply->appendErrorText("No access to path on disk! error: " + std::string(e.what()));
        // UiLog().warn() << "fetchLocalDir failed:" << std::string(e.what());
        fail();
    }

    finish();
}

//=================================
//
// OutputDirFetchTransferTask
//
//=================================

OutputDirFetchTransferTask::OutputDirFetchTransferTask(FetchQueueOwner* owner)
    : QObject(nullptr),
      OutputDirFetchTask("DirFetchTransfer", owner) {
}

OutputDirFetchTransferTask::~OutputDirFetchTransferTask() = default;

void OutputDirFetchTransferTask::stopTransfer() {
    if (transfer_) {
        dir_.reset();
        transfer_->stopTransfer(false);
    }
}

void OutputDirFetchTransferTask::stop() {
    stopTransfer();
    OutputDirFetchTask::clear();
}

void OutputDirFetchTransferTask::clear() {
    stopTransfer();
    OutputDirFetchTask::clear();
}

// Create an output client (to access the logserver) and ask it to the fetch the
// file asynchronously. The output client will call clientFinished() or
// clientError eventually!!
void OutputDirFetchTransferTask::run() {
#ifdef UI_OUTPUTDIRPROVIDER_TASK_DEBUG__
    UiLog().dbg() << UI_FN_INFO << "filePath=" << filePath_;
#endif

    assert(node_);
    assert(VConfig::instance()->proxychainsUsed());

    dir_.reset();

    auto socksPort = VFileTransfer::socksPort();
    if (socksPort.isEmpty()) {
        owner_->theReply()->addLogTryEntry("fetch from SOCKS host via ssh: NOT DEFINED");
        finish();
        return;
    }

    if (!transfer_) {
        transfer_ = new VDirTransfer(this);

        connect(transfer_, SIGNAL(transferFinished()), this, SLOT(transferFinished()));

        connect(transfer_, SIGNAL(transferFailed(QString)), this, SLOT(transferFailed(QString)));
    }

    Q_ASSERT(transfer_);
    transfer_->transferLocalViaSocks(QString::fromStdString(filePath_));
}

void OutputDirFetchTransferTask::transferFinished() {
    auto reply = owner_->theReply();
    reply->setInfoText("");
    reply->fileReadMode(VReply::TransferReadMode);

    if (transfer_) {
        auto resFile = transfer_->result();
        transfer_->clear();

        if (!resFile) {
            addTryLog(reply, "fetch from SOCKS host via ssh: FAILED");
            return;
        }

        QFile f(QString::fromStdString(resFile->path()));
        if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            addTryLog(reply, "fetch from SOCKS host via ssh: FAILED");
            return;
        }

        boost::filesystem::path fp(filePath_);
        std::string dirName = fp.parent_path().string();
        dir_                = std::make_shared<VDir>(dirName);
        dir_->setFetchMode(VDir::TransferFetchMode);
        dir_->setFetchModeStr(resFile->fetchModeStr());
        dir_->setFetchDate(resFile->fetchDate());

        while (!f.atEnd()) {
            QByteArray line = f.readLine();
            parseLine(line);
        }

        addTryLog(reply, "fetch from SOCKS host via ssh: OK");
        reply->appendDirectory(dir_);

        dir_.reset();
        succeed();
    }
    else {
        addTryLog(reply, "fetch from SOCKS host via ssh: FAILED");
    }
}

void OutputDirFetchTransferTask::transferProgress(QString /*msg*/, int /*value*/) {
}

void OutputDirFetchTransferTask::transferFailed(QString msg) {
    auto reply = owner_->theReply();
    addTryLog(reply, "fetch from SOCKS host via ssh: FAILED");
    reply->appendErrorText("Failed to fetch from SOCKS host via ssh\n" + msg.toStdString());
    transfer_->clear();
    fail();
}

void OutputDirFetchTransferTask::parseLine(QString line) {
    assert(dir_);
    line     = line.trimmed();
    auto lst = line.split(" ");
    if (lst.count() >= 3) {
        size_t mTime  = lst[0].toUInt();
        int fSize     = lst[1].toInt();
        QString fName = line.mid(lst[0].size() + 1 + lst[1].size() + 1);
        // UiLog().dbg() << UI_FN_INFO << fName << " " << fSize << " " << mTime;
        dir_->addItem(fName.toStdString(), fSize, mTime);
    }
}

class OutputDirFetchQueueManager : public FetchQueueOwner {
public:
    OutputDirFetchQueueManager(OutputDirProvider*);
    void run(ServerHandler* server, VNode* node, const std::string& joboutFile);

    VReply* theReply() const override;
    void fetchQueueSucceeded() override {}
    void fetchQueueFinished(const std::string& filePath, VNode*) override;

protected:
    OutputDirProvider* provider_{nullptr};
};

OutputDirFetchQueueManager::OutputDirFetchQueueManager(OutputDirProvider* provider) : provider_(provider) {
    fetchQueue_ = new FetchQueue(FetchQueue::RunAll, this);
}

VReply* OutputDirFetchQueueManager::theReply() const {
    return provider_->reply_;
}

void OutputDirFetchQueueManager::run(ServerHandler* server, VNode* node, const std::string& joboutFile) {
    // jobout
    fetchQueue_->clear();
    Q_ASSERT(fetchQueue_->isEmpty());
    AbstractFetchTask* t = nullptr;

    // jobout
    t = makeFetchTask("dir_logserver");
    t->reset(server, node, joboutFile);
    fetchQueue_->add(t);

    if (VConfig::instance()->proxychainsUsed()) {
        t = makeFetchTask("dir_transfer");
    }
    else {
        t = makeFetchTask("dir_local");
    }
    t->reset(server, node, joboutFile);
    if (!server->readFromDisk()) {
        t->setRunCondition(AbstractFetchTask::RunIfPrevFailed);
    }
    fetchQueue_->add(t);

    // jobfile
    std::string outFile = node->findVariable("ECF_OUT", true);
    std::string jobFile = node->findVariable("ECF_JOB", true);
    if (!outFile.empty() && !jobFile.empty()) {
        t = makeFetchTask("dir_logserver");
        t->reset(server, node, jobFile);
        fetchQueue_->add(t);

        if (VConfig::instance()->proxychainsUsed()) {
            t = makeFetchTask("dir_transfer");
        }
        else {
            t = makeFetchTask("dir_local");
        }
        t->reset(server, node, jobFile);
        if (!server->readFromDisk()) {
            t->setRunCondition(AbstractFetchTask::RunIfPrevFailed);
        }
        fetchQueue_->add(t);
    }

#ifdef UI_OUTPUTFILEPROVIDER_DEBUG__
    UiLog().dbg() << UI_FN_INFO << "queue=" << fetchQueue_;
#endif
    fetchQueue_->run();
}

void OutputDirFetchQueueManager::fetchQueueFinished(const std::string& /*filePath*/, VNode*) {
    theReply()->setInfoText("");
    if (theReply()->directories().empty()) {
        provider_->owner_->infoFailed(theReply());
    }
    else {
#ifdef UI_OUTPUTDIRPROVIDER_DEBUG__
        UiLog().dbg() << UI_FN_INFO << "dirs=" << reply_->directories().size();
#endif
        provider_->owner_->infoReady(theReply());
    }
}

//=================================
//
// OutputDirProvider
//
//=================================

OutputDirProvider::OutputDirProvider(InfoPresenter* owner) : InfoProvider(owner, VTask::NoTask) {
    fetchManager_ = new OutputDirFetchQueueManager(this);
}

OutputDirProvider::~OutputDirProvider() {
    if (fetchManager_) {
        delete fetchManager_;
        fetchManager_ = nullptr;
    }
}

void OutputDirProvider::clear() {
    // clear the queue and the fetch tasks
    if (fetchManager_) {
        fetchManager_->clear();
    }

    InfoProvider::clear();
}

// Node
void OutputDirProvider::visit(VInfoNode* info) {
    // Reset the reply
    reply_->reset();

    // Clear the queue
    fetchManager_->clear();

    if (!info) {
        owner_->infoFailed(reply_);
        return;
    }

    ServerHandler* server = info_->server();
    VNode* n              = info->node();

    if (!server || !n || !n->node()) {
        owner_->infoFailed(reply_);
        return;
    }

    std::string joboutFile = n->findVariable("ECF_JOBOUT", true);

    fetchManager_->run(server, n, joboutFile);
}

static FetchTaskMaker<OutputDirFetchLocalTask> maker1("dir_local");
static FetchTaskMaker<OutputDirFetchTransferTask> maker2("dir_transfer");
static FetchTaskMaker<OutputDirFetchLogServerTask> maker3("dir_logserver");
