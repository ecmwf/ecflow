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

#include "OutputDirClient.hpp"
#include "VNode.hpp"
#include "VReply.hpp"
#include "ServerHandler.hpp"
#include "UiLog.hpp"
#include "VConfig.hpp"
#include "VFileTransfer.hpp"

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <memory>

#include <QFile>

#define UI_OUTPUTDIRPROVIDER_DEBUG__
#define UI_OUTPUTDIRPROVIDER_TASK_DEBUG__

OutputDirFetchTask::OutputDirFetchTask(const std::string& name, OutputDirProvider* owner) :
    OutputFetchTask(name), owner_(owner)
{
}

void OutputDirFetchTask::reset(ServerHandler* server,VNode* node,const std::string& filePath,RunCondition cond)
{
    server_ = server;
    node_ = node;
    filePath_ = filePath;
    runCondition_ = cond;
}

void OutputDirFetchTask::addTryLog(VReply* r, const std::string& txt) const
{
    r->addLogTryEntry("<PATH>" + filePath_ + "</PATH> " +  txt);
}

//=================================
//
// OutputDirFetchRemoteTask
//
//=================================

OutputDirFetchRemoteTask::OutputDirFetchRemoteTask(OutputDirProvider* owner) :
     QObject(nullptr), OutputDirFetchTask("DirFetchRemote", owner)
{
}

OutputDirFetchRemoteTask::~OutputDirFetchRemoteTask()
{
    if(client_) {
        client_->disconnect(this);
    }
}

void OutputDirFetchRemoteTask::deleteClient()
{
    if(client_) {
#ifdef UI_OUTPUTDIRPROVIDER_TASK_DEBUG__
        UI_FN_DBG
#endif
        client_->disconnect(this);
        client_->deleteLater();
        client_ = nullptr;
    }
}

void OutputDirFetchRemoteTask::stop()
{
    OutputFetchTask::clear();
    if (status_ == RunningStatus) {
        deleteClient();
    }
}


void OutputDirFetchRemoteTask::clear()
{
    OutputFetchTask::clear();
    deleteClient();
    if(client_) {
        delete client_;
        client_ = nullptr;
    }
}

//Create an output client (to access the logserver) and ask it to the fetch the
//file asynchronously. The output client will call clientFinished() or
//clientError eventually!!
void OutputDirFetchRemoteTask::run()
{
#ifdef UI_OUTPUTDIRPROVIDER_TASK_DEBUG__
    UiLog().dbg() <<  UI_FN_INFO << "filePath=" << filePath_;
#endif
    std::string host, port;
    assert(node_);

    // First try the user defined logserver, then the system defined one
    bool userLogServerUsed = node_->userLogServer(host,port);
    bool sysLogServerUsed = false;
    if (!userLogServerUsed) {
        sysLogServerUsed = node_->logServer(host,port);
    }
    Q_ASSERT(!userLogServerUsed || !sysLogServerUsed);

#ifdef UI_OUTPUTDIRPROVIDER_TASK_DEBUG__
    UiLog().dbg() <<  UI_FN_INFO << "host=" << host << " port=" << port;
#endif

    if (userLogServerUsed || sysLogServerUsed) {
        Q_ASSERT(userLogServerUsed || sysLogServerUsed);
        if (client_ && (client_->host() != host || client_->portStr() != port)) {
            deleteClient();
        }

        if (!client_) {
            client_=new OutputDirClient(host,port,this);

            connect(client_,SIGNAL(error(QString)),
                    this,SLOT(clientError(QString)));

            connect(client_,SIGNAL(progress(QString,int)),
                    this,SLOT(clientProgress(QString,int)));

            connect(client_,SIGNAL(finished()),
                    this,SLOT(clientFinished()));
        }

        Q_ASSERT(client_);
//        UiLog().dbg() << "OutputFileProvider: logserver=" << client_->longName() << " file=" << filePath_;
//        owner_->owner_->infoProgressStart("Getting file <i>" + filePath_ +  "</i> from" +
//                                  ((userLogServerUsed)?"<b>user defined</b>":"") +
//                                  " log server <i> " + client_->longName() + "</i>",0);


        // fetch the file asynchronously
        status_ = RunningStatus;
        client_->getDir(filePath_);
        return;
    }


    //If we are here there is no output client defined/available
    deleteClient();

    addTryLog(owner_->reply_, "fetch from logserver: NOT DEFINED");
    finish();
}

void OutputDirFetchRemoteTask::clientFinished()
{
    Q_ASSERT(client_);
    auto reply = owner_->reply_;
    VDir_ptr dir = client_->result();
    if(dir)
    {
#ifdef UI_OUTPUTDIRPROVIDER_TASK_DEBUG__
        UI_FN_DBG
#endif
        dir->setFetchMode(VDir::LogServerFetchMode);
        std::string method="served by " + client_->longName();
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

void OutputDirFetchRemoteTask::clientProgress(QString,int)
{
}

void OutputDirFetchRemoteTask::clientError(QString msg)
{
    std::string sDesc;
    auto reply = owner_->reply_;
    if(client_)
    {
        sDesc="Failed to fetch from " + client_->longName();
        if(!msg.isEmpty())
            sDesc+=" error: " + msg.toStdString();

    }
    else
    {
        sDesc="Failed to fetch from logserver";
        if(!msg.isEmpty())
            sDesc+=": " + msg.toStdString();;
    }

    addTryLog(reply, "fetch file from logserver=" + client_->longName() + " : FAILED");
    reply->appendErrorText(sDesc);
    fail();
}

//=================================
//
// OutputFileFetchLocalTask
//
//=================================
OutputDirFetchLocalTask::OutputDirFetchLocalTask(OutputDirProvider *owner) :
    OutputDirFetchTask("DirFetchLocal", owner) {}

// try to read the logfile from the disk (if the settings allow it)
void OutputDirFetchLocalTask::run()
{
#ifdef UI_OUTPUTDIRPROVIDER_TASK_DEBUG__
    UI_FN_DBG
#endif
    VDir_ptr res;

    boost::filesystem::path p(filePath_);

    //Is it a directory?
    boost::system::error_code errorCode;
    if (boost::filesystem::is_directory(p,errorCode)) {
        fail();
        return;
    }

    auto reply = owner_->reply_;
    try {
        if(boost::filesystem::exists(p.parent_path())) {
            std::string dirName=p.parent_path().string();
            //if(info_ && info_->isNode() && info_->node())
            if (node_) {
                std::string nodeName=node_->strName();
                std::string pattern=nodeName+".";
                res=std::make_shared<VDir>(dirName,pattern);
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
    } catch (const boost::filesystem::filesystem_error& e) {
        addTryLog(reply, "read from disk: NO ACCESS");
        reply->appendErrorText("No access to path on disk! error: " + std::string(e.what()));
        UiLog().warn() << "fetchLocalDir failed:" << std::string(e.what());
        fail();
    }

    finish();
}

//=================================
//
// OutputDirFetchTransferTask
//
//=================================

OutputDirFetchTransferTask::OutputDirFetchTransferTask(OutputDirProvider* owner) :
     QObject(nullptr), OutputDirFetchTask("DirFetchTransfer", owner)
{
}

OutputDirFetchTransferTask::~OutputDirFetchTransferTask()
{
}

void OutputDirFetchTransferTask::stopTransfer()
{
    if (transfer_) {
        dir_.reset();
        resFile_.reset();
        transfer_->stopTransfer(false);
     }
}

void OutputDirFetchTransferTask::stop()
{
    stopTransfer();
    OutputDirFetchTask::clear();
}

void OutputDirFetchTransferTask::clear()
{
    stopTransfer();
    OutputDirFetchTask::clear();
}

//Create an output client (to access the logserver) and ask it to the fetch the
//file asynchronously. The output client will call clientFinished() or
//clientError eventually!!
void OutputDirFetchTransferTask::run()
{
#ifdef  UI_OUTPUTFILEPROVIDER_TASK_DEBUG__
    UiLog().dbg() << UI_FN_INFO << "filePath=" << filePath_;
#endif

    assert(node_);
    assert(VConfig::instance()->proxychainsUsed());

    dir_.reset();
    resFile_.reset();

    QString rUser, rHost;
    VFileTransfer::socksRemoteUserAndHost(rUser, rHost);
    if (rUser.isEmpty() || rHost.isEmpty()) {
        owner_->reply_->addLogTryEntry("fetch from SOCKS host via ssh: NOT DEFINED");
        finish();
        return;
    }


    resFile_ = VFile::createTmpFile(true);  //we will delete the file from disk

    if (!transfer_) {
        transfer_ = new VDirTransfer(this);

        connect(transfer_, SIGNAL(transferFinished()),
                this, SLOT(transferFinished()));

        connect(transfer_, SIGNAL(transferFailed(QString)),
                this, SLOT(transferFailed(QString)));
    }

    Q_ASSERT(transfer_);
    transfer_->transferLocalViaSocks(QString::fromStdString(filePath_),
                       QString::fromStdString(resFile_->path()));


}

void OutputDirFetchTransferTask::transferFinished()
{
    auto reply = owner_->reply_;
    reply->setInfoText("");
    reply->fileReadMode(VReply::TransferReadMode);

    QFile f(QString::fromStdString(resFile_->path()));
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        addTryLog(reply, "fetch from SOCKS host via ssh: FAILED");
        return;
    }

    boost::filesystem::path fp(filePath_);
    std::string dirName=fp.parent_path().string();
    dir_=std::make_shared<VDir>(dirName);

    while (!f.atEnd()) {
        QByteArray line = f.readLine();
        parseLine(line);
    }

    dir_->setFetchMode(VDir::TransferFetchMode);
    std::string method="via ssh";
    if (transfer_) {
        method = "from " + transfer_->remoteUserAndHost().toStdString() + " " + method;
    }
    dir_->setFetchModeStr(method);
    dir_->setFetchDate(QDateTime::currentDateTime());

    addTryLog(reply, "fetch from SOCKS host via ssh: OK");
    reply->appendDirectory(dir_);

    resFile_.reset();
    dir_.reset();

    succeed();
}

void OutputDirFetchTransferTask::transferProgress(QString msg,int value)
{
    owner_->owner_->infoProgressUpdate(msg.toStdString(),value);
}

void OutputDirFetchTransferTask::transferFailed(QString msg)
{
    auto reply = owner_->reply_;
#ifdef  UI_OUTPUTFILEPROVIDER_TASK_DEBUG__
    UiLog().dbg() << UI_FN_INFO << "msg=" << msg;
#endif
    addTryLog(reply, "fetch from SOCKS host via ssh: FAILED");
    reply->appendErrorText("Failed to fetch via ssh\n");
    resFile_.reset();
    fail();
}

void OutputDirFetchTransferTask::parseLine(QString line)
{
    assert(dir_);
    line = line.trimmed();
    auto lst = line.split(" ");
    if (lst.count()>=3) {
        size_t mTime = lst[0].toUInt();
        int fSize = lst[1].toInt();
        QString fName = line.mid(lst[0].count()+1+lst[1].count()+1);
        UiLog().dbg() << UI_FN_INFO << fName << " " << fSize << " " << mTime;
        dir_->addItem(fName.toStdString(),fSize,mTime);
    }
}

//=================================
//
// OutputDirProvider
//
//=================================

OutputDirProvider::OutputDirProvider(InfoPresenter* owner) :
    InfoProvider(owner,VTask::NoTask)
{   
    fetchQueue_ = new OutputFetchQueue(OutputFetchQueue::RunAll, this);

    // these are persistent fetch tasks. We add them to the queue on demand
    fetchTask_[LocalTask1] = new OutputDirFetchLocalTask(this);
    fetchTask_[RemoteTask1] = new OutputDirFetchRemoteTask(this);
    fetchTask_[TransferTask1] = new OutputDirFetchTransferTask(this);
    fetchTask_[LocalTask2] = new OutputDirFetchLocalTask(this);
    fetchTask_[RemoteTask2] = new OutputDirFetchRemoteTask(this);
    fetchTask_[TransferTask2] = new OutputDirFetchTransferTask(this);
}

OutputDirProvider::~OutputDirProvider()
{
    delete fetchQueue_;
    fetchQueue_ = nullptr;

    for (auto it: fetchTask_) {
        delete it.second;
    }
    fetchTask_.clear();
}

void OutputDirProvider::clear()
{
    // clear the queue and the fetch tasks
    if (fetchQueue_) {
        fetchQueue_->clear();
    }

	InfoProvider::clear();
}

//Node
void OutputDirProvider::visit(VInfoNode* info)
{
	//Reset the reply
	reply_->reset();

    //Clear the queue
    fetchQueue_->clear();

	if(!info)
 	{
       	owner_->infoFailed(reply_);
        return;
   	}

    ServerHandler* server=info_->server();
	VNode *n=info->node();

    if(!server || !n || !n->node())
   	{
       	owner_->infoFailed(reply_);
        return;
   	}

    std::string joboutFile=n->findVariable("ECF_JOBOUT",true);

    // jobout
    auto t = fetchTask_[RemoteTask1];
    t->reset(server,n,joboutFile);
    fetchQueue_->add(t);

    if (VConfig::instance()->proxychainsUsed()) {
        t = fetchTask_[TransferTask1];
    } else {
        t = fetchTask_[LocalTask1];
    }
    t->reset(server,n,joboutFile);
    if(server->readFromDisk()) {
        fetchQueue_->add(t);
    } else {
        t->setRunCondition(OutputFetchTask::RunIfPrevFailed);
        fetchQueue_->add(t);
    }

    // jobfile
    std::string outFile = n->findVariable("ECF_OUT",true);
    std::string jobFile = n->findVariable("ECF_JOB",true);
    if(!outFile.empty() && !jobFile.empty()) {
        t = fetchTask_[RemoteTask2];
        t->reset(server,n,jobFile);
        fetchQueue_->add(t);

        if (VConfig::instance()->proxychainsUsed()) {
            t = fetchTask_[TransferTask2];
        } else {
            t = fetchTask_[LocalTask2];
        }
        t->reset(server,n,jobFile);
        if(server->readFromDisk()) {
            fetchQueue_->add(t);
        } else {
            t->setRunCondition(OutputFetchTask::RunIfPrevFailed);
            fetchQueue_->add(t);
        }
    }
    fetchQueue_->run();
}

void OutputDirProvider::fetchQueueSucceeded()
{
}

void OutputDirProvider::fetchQueueFinished(const std::string& /*filePath*/, VNode*)
{
#ifdef UI_OUTPUTDIRPROVIDER_DEBUG__
    UI_FN_DBG
#endif
    reply_->setInfoText("");
    if (reply_->directories().empty()) {
        owner_->infoFailed(reply_);
    } else {
#ifdef UI_OUTPUTDIRPROVIDER_DEBUG__
        UiLog().dbg() << " dirs=" << reply_->directories().size();
#endif
        owner_->infoReady(reply_);
    }
}
