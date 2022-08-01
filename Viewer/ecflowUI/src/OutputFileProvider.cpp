//============================================================================
// Copyright 2009- ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "OutputFileProvider.hpp"

#include <deque>

#include "OutputCache.hpp"
#include "OutputFileClient.hpp"
#include "VNode.hpp"
#include "VReply.hpp"
#include "ServerHandler.hpp"
#include "UiLog.hpp"

#include <QDateTime>

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/algorithm/string/predicate.hpp>

#define DEBUG_OUTPUTFILEPROVIDER__

//=================================
//
// OutputFileFetchTask
//
//=================================

OutputFileFetchTask::OutputFileFetchTask(OutputFileProvider* owner) :
    owner_(owner)
{
    Q_ASSERT(owner_);
}

void OutputFileFetchTask::clear()
{
    OutputFetchTask::clear();
    isJobout_=false;
    deltaPos_=0;
    useCache_=false;
}

void OutputFileFetchTask::reset(ServerHandler* server,VNode* node,const std::string& filePath, bool isJobout,
           size_t deltaPos, bool useCache)
{
    server_=server;
    node_=node;
    filePath_=filePath;
    isJobout_=isJobout;
    deltaPos_=deltaPos;
    useCache_=useCache;
}

//=================================
//
// OutputFileFetchCacheTask
//
//=================================

// Try to fetch the logfile from the local cache
void OutputFileFetchCacheTask::run()
{
#ifdef  DEBUG_OUTPUTFILEPROVIDER__
    UI_FUNCTION_LOG
    UiLog().dbg() << "OutputFileFetchCacheTask::run <-- filePath: " << filePath_;
#endif

    assert(node_);

    //We try use the cache
    if(useCache_)
    {
        //Check if the given output is already in the cache
        if(OutputCacheItem* item=owner_->findInCache(filePath_))
        {
            VFile_ptr f=item->file();
            assert(f);
            f->setCached(true);

            UiLog().dbg() << "  File found in cache";

            auto reply = owner_->reply_;
            reply->setInfoText("");
            reply->fileReadMode(VReply::LogServerReadMode);
            reply->setLog(f->log());
            reply->addLog("REMARK>File was read from cache.");
            reply->tmpFile(f);

            succeed();
            return;
        }
    }
    finish();
}

//=================================
//
// OutputFileFetchRemoteTask
//
//=================================

OutputFileFetchRemoteTask::OutputFileFetchRemoteTask(OutputFileProvider* owner) :
     QObject(nullptr), OutputFileFetchTask(owner)
{
}

void OutputFileFetchRemoteTask::stop()
{
    clear();
}

void OutputFileFetchRemoteTask::clear()
{
    OutputFileFetchTask::clear();
    if(client_) {
        delete client_;
        client_ = nullptr;
    }
}

//Create an output client (to access the logserver) and ask it to the fetch the
//file asynchronously. The output client will call clientFinished() or
//clientError eventually!!
void OutputFileFetchRemoteTask::run()
{
#ifdef  DEBUG_OUTPUTFILEPROVIDER__
    UI_FUNCTION_LOG
    UiLog().dbg() << "OutputFileFetchRemoteTask::run <-- file: " << filePath_;
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
            delete client_;
            client_ = nullptr;
        }

        if (!client_) {
            client_=new OutputFileClient(host,port,this);

            connect(client_,SIGNAL(error(QString)),
                    this,SLOT(clientError(QString)));

            connect(client_,SIGNAL(progress(QString,int)),
                    this,SLOT(clientProgress(QString,int)));

            connect(client_,SIGNAL(finished()),
                    this,SLOT(clientFinished()));
        }

        Q_ASSERT(client_);
        UiLog().dbg() << "OutputFileFetchRemoteTask: use logserver=" << client_->longName() << " file=" << filePath_;
        owner_->owner_->infoProgressStart("Getting file <i>" + filePath_ +  "</i> from" +
                                  ((userLogServerUsed)?"<b>user defined</b>":"") +
                                  " log server <i> " + client_->longName() + "</i>",0);


        VDir_ptr dir=owner_->dirToFile(filePath_);
        client_->setDir(dir);

        // fetch the file asynchronously
        client_->getFile(filePath_, deltaPos_);
        return;
    }


    //If we are here there is no output client defined/available
    if (client_) {
        delete client_;
        client_ = nullptr;
    }

    owner_->reply_->addLog("TRY>fetch file from logserver: NOT DEFINED");
    finish();
}

void OutputFileFetchRemoteTask::clientFinished()
{
    VFile_ptr tmp = client_->result();
    assert(tmp);

    client_->clearResult();

    //Files retrieved from the log server are automatically added to the cache!
    if (useCache_ && !tmp->hasDeltaContents()) {
        owner_->addToCache(tmp);
    }

    auto reply = owner_->reply_;
    reply->setInfoText("");
    reply->fileReadMode(VReply::LogServerReadMode);

    if (tmp->hasDeltaContents()) {
        reply->addLog("TRY> fetch delta from logserver: " + client_->longName() + ": OK");
    } else {
        reply->addLog("TRY> fetch file from logserver: " + client_->longName() + ": OK");
    }

    tmp->setFetchMode(VFile::LogServerFetchMode);
    tmp->setLog(reply->log());
    std::string method="served by " + client_->longName();
    tmp->setFetchModeStr(method);

    reply->tmpFile(tmp);

    succeed();
}

void OutputFileFetchRemoteTask::clientProgress(QString msg,int value)
{
    //UiLog().dbg() << "OutputFileProvider::slotOutputClientProgress " << msg;

    owner_->owner_->infoProgressUpdate(msg.toStdString(),value);

    //reply_->setInfoText(msg.toStdString());
    //owner_->infoProgress(reply_);
    //reply_->setInfoText("");
}

void OutputFileFetchRemoteTask::clientError(QString msg)
{
    auto reply = owner_->reply_;
    UiLog().dbg() << "OutputFileProvider::slotOutputClientError error:" << msg;
    reply->addLog("TRY> fetch file from logserver: " + client_->longName() + " FAILED");
    fail();
}

//=================================
//
// OutputFileFetchLocalTask
//
//=================================

// try to read the logfile from the disk (if the settings allow it)
void OutputFileFetchAnyLocalTask::run()
{
    auto reply = owner_->reply_;

    //we do not want to delete the file once the VFile object is destroyed!!
    VFile_ptr f(VFile::create(filePath_,false));
    if(f->exists())
    {
        reply->fileReadMode(VReply::LocalReadMode);
        reply->addLog("TRY> read file from disk: OK");

        f->setSourcePath(f->path());
        f->setFetchMode(VFile::LocalFetchMode);
        f->setFetchDate(QDateTime::currentDateTime());
        f->setLog(reply->log());

        reply->tmpFile(f);

        succeed();
        return ;
    }
    reply->addLog("TRY> read file from disk: NO ACCESS");
    finish();
}

void OutputFileFetchLocalTask::run()
{
    if (server_->readFromDisk() || !isJobout_) {
        OutputFileFetchAnyLocalTask::run();
    } else {
        finish();
    }
}

//=================================
//
// OutputFileFetchServerTask
//
//=================================

// try to fetch the logfile from the server if it is the jobout file
void OutputFileFetchServerTask::run()
{
    if (isJobout_) {
        // we delegate it back to the FileProvider (this is its built-in task)
        owner_->fetchJoboutViaServer(server_,node_,filePath_);
    } else {
        finish();
    }
}

//========================================
//
// OutputFileProvider
//
//========================================

OutputFileProvider::OutputFileProvider(InfoPresenter* owner) :
    InfoProvider(owner,VTask::OutputTask)
{
    outCache_=new OutputCache(this);

    fetchQueue_ = new OutputFetchQueue(OutputFetchQueue::RunUntilFirstSucceed, this);

    // these are persistent fetch tasks. We add them to the queue on demand
    fetchTask_[CacheTask] = new OutputFileFetchCacheTask(this);
    fetchTask_[RemoteTask] = new OutputFileFetchRemoteTask(this);
    fetchTask_[AnyLocalTask] = new OutputFileFetchAnyLocalTask(this);
    fetchTask_[LocalTask] = new OutputFileFetchLocalTask(this);
    fetchTask_[ServerTask] = new OutputFileFetchServerTask(this);
}

OutputFileProvider::~OutputFileProvider()
{
    delete fetchQueue_;
    fetchQueue_ = nullptr;

    for (auto it: fetchTask_) {
        delete it.second;
    }
    fetchTask_.clear();
}

// This is called from the destructor
void OutputFileProvider::clear()
{
    //Detach all the outputs registered for this instance in cache
    outCache_->detach();

    // clear the queue and the fetch tasks
    if (fetchQueue_) {
        fetchQueue_->clear();
        for (auto it: fetchTask_) {
            it.second->clear();
        }
    }

    InfoProvider::clear();
    dirs_.clear();
}

//Check if the given output is already in the cache
OutputCacheItem* OutputFileProvider::findInCache(const std::string& fileName)
{
    return outCache_->attachOne(info_,fileName);
}

void OutputFileProvider::addToCache(VFile_ptr file)
{
    outCache_->add(info_,file->sourcePath(), file);
}

//This is called when we load a new node in the Output panel. In this
//case we always try to load the current jobout file.
void OutputFileProvider::visit(VInfoNode* infoNode)
{
    assert(info_->node() == infoNode->node());

    // clear the queue
    fetchQueue_->clear();

    //Reset the reply
	reply_->reset();

    if(!info_)
 	{
       	owner_->infoFailed(reply_);
        return;
   	}

	ServerHandler* server=info_->server();
    VNode *n=infoNode->node();

    if(!n || !n->node())
   	{
       	owner_->infoFailed(reply_);
        return;
   	}

    //Get the filename
    std::string jobout=joboutFileName();

    //We always try to use the cache in this case
    outCache_->attachOne(info_,jobout);
    fetchFile(server,n,jobout,true,0,true);
}

//Get a file
void OutputFileProvider::file(const std::string& fileName, size_t deltaPos, bool useCache)
{
    // clear the queue
    fetchQueue_->clear();

    //If we do not want to use the cache we detach all the output
    //attached to this instance
    if(!useCache)
        outCache_->detach();

    //Check if the task is already running
	if(task_)
	{
		task_->status(VTask::CANCELLED);
		task_.reset();
	}

	//Reset the reply
	reply_->reset();

	if(!info_->isNode() || !info_->node() || !info_->node()->node())
	{
	    owner_->infoFailed(reply_);
        return;
	}

	ServerHandler* server=info_->server();
	VNode *n=info_->node();

    //Get the filename
    std::string jobout=joboutFileName();

    fetchFile(server,n,fileName,(fileName==jobout), deltaPos, useCache);
}

void OutputFileProvider::fetchFile(ServerHandler *server,VNode *n,const std::string& fileName,bool isJobout, size_t deltaPos, bool useCache)
{
    //If we do not want to use the cache we detach all the output
    //attached to this instance
    if(!useCache)
        outCache_->detach();

    if(!n || !n->node() || !server)
    {
    	owner_->infoFailed(reply_);
    	return;
    }

	//Set the filename in reply
	reply_->fileName(fileName);

	//No filename is available
	if(fileName.empty())
	{
		//Joubout variable is not defined or empty
		if(isJobout)
		{
			reply_->setErrorText("Variable ECF_JOBOUT is not defined!");
            reply_->addLog("MSG>Variable ECF_JOBOUT is not defined!.");
            owner_->infoFailed(reply_);
		}
		else
		{
			reply_->setErrorText("Output file is not defined!");
            reply_->addLog("MSG>Output file is not defined.");
            owner_->infoFailed(reply_);
		}
		return;
	}

    //Check if tryno is 0. ie. the file is the current jobout file and ECF_TRYNO = 0
    if(isTryNoZero(fileName))
    {
        reply_->setInfoText("Current job output does not exist yet (<b>TRYNO</b> is <b>0</b>)!)");
        reply_->addLog("MSG>Current job output does not exist yet (<b>TRYNO</b> is <b>0</b>)!");
        owner_->infoReady(reply_);
        return;
    }

    //----------------------------------
    // The host is the localhost
    //----------------------------------

    if(isJobout)
    {
        if(n->isSubmitted())
            reply_->addLog("REMARK>This file is the <b>current</b> job output (defined by variable <b>ECF_JOBOUT</b>), but \
                  because the node status is <b>submitted</b> it may contain the output from a previous run!");
        else
            reply_->addLog("REMARK>This file is the <b>current</b> job output (defined by variable <b>ECF_JOBOUT</b>).");
    }
    else
    {
        reply_->addLog("REMARK>This file is <b>not</b> the <b>current</b> job output (defined by <b>ECF_JOBOUT</b>).");
    }

    // Update the fetch tasks and process them. The queue runs until any task can fetch
    // the logfile
    fetchQueue_->clear();
    Q_ASSERT(fetchQueue_->isEmpty());
    QList<FetchTaskType> types = {CacheTask, RemoteTask, LocalTask, ServerTask};
    for (auto k: types) {
        auto t = fetchTask_[k];
        t->reset(server,n,fileName,isJobout, deltaPos, useCache);
        fetchQueue_->add(t);
    }
    fetchQueue_->run();
}

//Get a file with the given fetch mode. We use it to fettch files appearing in the directory
//listing in the Output panel.
void OutputFileProvider::fetchFile(const std::string& fileName,VDir::FetchMode fetchMode,size_t deltaPos, bool useCache)
{
    //If we do not want to use the cache we detach all the output
    //attached to this instance
    if(!useCache)
        outCache_->detach();

    //Check if the task is already running
    if(task_)
    {
        task_->status(VTask::CANCELLED);
        task_.reset();
    }

    //Reset the reply
    reply_->reset();

    if(!info_->isNode() || !info_->node() || !info_->node()->node())
    {
        owner_->infoFailed(reply_);
        return;
    }

    ServerHandler* server=info_->server();
    VNode *node=info_->node();

    //Get the filename
    std::string jobout=joboutFileName(); //n->findVariable("ECF_JOBOUT",true);
    bool isJobout=(fileName==jobout);

    //Set the filename in reply
    reply_->fileName(fileName);

    //No filename is available
    if(fileName.empty())
    {
        reply_->setErrorText("Output file is not defined!");
        reply_->addLog("MSG>Output file is not defined.");
        owner_->infoFailed(reply_);
        return;
    }

    //Check if tryno is 0. ie. the file is the current jobout file and ECF_TRYNO = 0
    if(isTryNoZero(fileName))
    {
        reply_->setInfoText("Current job output does not exist yet (<b>TRYNO</b> is <b>0</b>)!)");
        reply_->addLog("MSG>Current job output does not exist yet (<b>TRYNO</b> is <b>0</b>)!");
        owner_->infoReady(reply_);
        return;
    }

    fetchQueue_->clear();

    OutputFileFetchTask *t=nullptr;
    if (fetchMode == VDir::LogServerFetchMode) {
        t = fetchTask_[RemoteTask];
    } else if(fetchMode == VDir::LocalFetchMode) {
        t = fetchTask_[AnyLocalTask];
    } else if(isJobout && fetchMode == VDir::ServerFetchMode) {
        t = fetchTask_[ServerTask];
    }

    if (t) {
        t->reset(server,node,fileName,isJobout, deltaPos, useCache);
        fetchQueue_->add(t);
    }

    fetchQueue_->run();
}

void OutputFileProvider::fetchQueueSucceeded()
{
    owner_->infoReady(reply_);
}

void OutputFileProvider::fetchQueueFinished(VNode* n)
{
    if(n && n->isFlagSet(ecf::Flag::JOBCMD_FAILED))
    {
       reply_->setErrorText("Submission command failed! Check .sub file, ssh, or queueing system error.");
    }
    owner_->infoFailed(reply_);
}

void OutputFileProvider::fetchJoboutViaServer(ServerHandler *server,VNode *n,const std::string& fileName)
{
    // this must be called from the queue and must be the last task of the queue. From this moment on we do
    // not need the queue and the OutputFileProvider manages the VTask
    fetchQueue_->clear();

    assert(server);
    assert(n);

    //Define a task for getting the info from the server.
    task_=VTask::create(taskType_,n,this);

    task_->reply()->fileReadMode(VReply::ServerReadMode);
    task_->reply()->fileName(fileName);
    task_->reply()->setLog(reply_->log());

    //owner_->infoProgressStart("Getting file <i>" + fileName + "</i> from server",0);

    //Run the task in the server. When it finish taskFinished() is called. The text returned
    //in the reply will be prepended to the string we generated above.
    server->run(task_);
}

std::string OutputFileProvider::joboutFileName() const
{
    if(info_ && info_->isNode() && info_->node() && info_->node()->node())
	{
		return info_->node()->findVariable("ECF_JOBOUT",true);
	}

	return std::string();
}

//Returns true if
//   -the file is the current jobout
//   -id contains the string ".0"
//   -ECF_TRYNO = 0

bool OutputFileProvider::isTryNoZero(const std::string& filename) const
{
    if(filename.find(".0") != std::string::npos &&
       joboutFileName() == filename &&
       info_ && info_->isNode() && info_->node() && info_->node()->node())
    {
        return (info_->node()->findVariable("ECF_TRYNO",true) == "0");
    }
    return false;
}

//We use directories to figure out the size of the file to be transfered
void OutputFileProvider::setDirectories(const std::vector<VDir_ptr>& dirs)
{
#if 0
    if(outClient_)
        outClient_->setDir(dir);  

    if(dir != dir_)
        dir_=dir;
#endif

    dirs_=dirs;
}

VDir_ptr OutputFileProvider::dirToFile(const std::string& fileName) const
{
    VDir_ptr dir;

    if(fileName.empty())
       return dir;

    for(const auto & dir : dirs_)
    {
        if(dir && fileName.find(dir->path()) == 0)
            return dir;

    }
    return dir;
}
