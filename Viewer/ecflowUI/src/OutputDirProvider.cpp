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

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <memory>

#define UI_OUTPUTDIRPROVIDER_DEBUG__
#define UI_OUTPUTDIRPROVIDER_TASK_DEBUG__

OutputDirFetchTask::OutputDirFetchTask(OutputDirProvider* owner) :
    owner_(owner)
{
    Q_ASSERT(owner_);
}

void OutputDirFetchTask::reset(ServerHandler* server,VNode* node,const std::string& filePath,RunCondition cond)
{
    server_ = server;
    node_ = node;
    filePath_ = filePath;
    runCondition_ = cond;
}

//=================================
//
// OutputFileFetchLocalTask
//
//=================================

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

                reply->appendDirectory(res);
                succeed();
                return;
            }
        }
        reply->appendErrorText("No access to path on disk!");
    } catch (const boost::filesystem::filesystem_error& e) {
        reply->appendErrorText("No access to path on disk! error: " + std::string(e.what()));
        UiLog().warn() << "fetchLocalDir failed:" << std::string(e.what());
        fail();
    }

    finish();
}

//=================================
//
// OutputDirFetchRemoteTask
//
//=================================

OutputDirFetchRemoteTask::OutputDirFetchRemoteTask(OutputDirProvider* owner) :
     QObject(nullptr), OutputDirFetchTask(owner)
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

    //owner_->reply_->addLog("TRY>fetch file from logserver: NOT DEFINED");
    finish();
}

void OutputDirFetchRemoteTask::clientFinished()
{
    Q_ASSERT(client_);
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

        owner_->reply_->appendDirectory(dir);
        succeed();
        return;
    }

    fail();
}

void OutputDirFetchRemoteTask::clientProgress(QString,int)
{
}

void OutputDirFetchRemoteTask::clientError(QString msg)
{
    std::string sDesc;
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

    owner_->reply_->appendErrorText(sDesc);
    fail();
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
    fetchTask_[LocalTask2] = new OutputDirFetchLocalTask(this);
    fetchTask_[RemoteTask2] = new OutputDirFetchRemoteTask(this);
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

    t = fetchTask_[LocalTask1];
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

        t = fetchTask_[LocalTask2];
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

void OutputDirProvider::fetchQueueFinished(VNode*)
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
