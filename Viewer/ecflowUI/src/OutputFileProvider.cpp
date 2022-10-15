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
#include "VFileTransfer.hpp"
#include "ServerHandler.hpp"
#include "UiLog.hpp"
#include "VConfig.hpp"

#include <QDateTime>

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/algorithm/string/predicate.hpp>

//#define UI_OUTPUTFILEPROVIDER_TASK_DEBUG__
//#define UI_OUTPUTFILEPROVIDER_DEBUG__

//=================================
//
// OutputFileFetchServerTask
//
//=================================

OutputFileFetchServerTask::OutputFileFetchServerTask(FetchQueueOwner* owner) :
    AbstractFetchTask("FileFetchServer", owner) {}

// try to fetch the logfile from the server if it is the jobout file
void OutputFileFetchServerTask::run()
{
#ifdef  UI_OUTPUTFILEPROVIDER_TASK_DEBUG__
    UiLog().dbg() << UI_FN_INFO << "filePath=" << filePath_;
#endif   
    // we delegate it back to the FileProvider (this is its built-in task)
    std::string fPath = filePath_;
    fileProvider_->fetchJoboutViaServer(server_,node_,fPath);
}

//========================================
//
// OutputFileFetchQueueManager
//
//========================================

class OutputFileFetchQueueManager : public FetchQueueOwner
{
public:
     OutputFileFetchQueueManager(OutputFileProvider*);
     void runFull(ServerHandler* server, VNode* node,
                  const std::string& fileName, bool isJobout,
                  size_t deltaPos, unsigned int modTime, const std::string& checkSum, bool useCache);
     void runMode(VFile::FetchMode fetchMode, ServerHandler* server, VNode* node,
                  const std::string& fileName, bool isJobout,
                  size_t deltaPos, unsigned int modTime, const std::string& checkSum, bool useCache);

     VReply* theReply() const override;
     VFile_ptr findInCache(const std::string& fileName) override;
     void addToCache(VFile_ptr file) override;
     void fetchQueueSucceeded() override;
     void fetchQueueFinished(const std::string& filePath, VNode*) override;
     void progressStart(const std::string& msg, int max) override;
     void progressUpdate(const std::string& msg, int value) override;
     void progressStop() override;
     VDir_ptr dirToFile(const std::string& fileName) const override;

protected:
     OutputFileProvider* provider_{nullptr};

};

OutputFileFetchQueueManager::OutputFileFetchQueueManager(OutputFileProvider* provider) : provider_(provider)
{
    fetchQueue_ = new FetchQueue(FetchQueue::RunUntilFirstSucceeded, this);
}

VReply* OutputFileFetchQueueManager::theReply() const
{
    return provider_->reply_;
}


//Check if the given output is already in the cache
VFile_ptr OutputFileFetchQueueManager::findInCache(const std::string& fileName)
{
    auto item = provider_->findInCache(fileName);
    return (item)?item->file():nullptr;
}

void OutputFileFetchQueueManager::addToCache(VFile_ptr file)
{
    provider_->addToCache(file);
}

void OutputFileFetchQueueManager::runFull(ServerHandler* server, VNode* node,
    const std::string& fileName, bool isJobout,
    size_t deltaPos, unsigned int modTime, const std::string& checkSum, bool useCache)
{
    // Update the fetch tasks and process them. The queue runs until any task can fetch
    // the logfile
    fetchQueue_->clear();
    Q_ASSERT(fetchQueue_->isEmpty());

    QList<AbstractFetchTask*> taskLst;

    taskLst << makeFetchTask("file_cache");
    taskLst << makeFetchTask("file_logserver");
    if (VConfig::instance()->proxychainsUsed()) {
        taskLst << makeFetchTask("file_transfer");
    } else {
        if (server->readFromDisk() || !isJobout) {
            taskLst << makeFetchTask("file_local");
        }
    }
    if (isJobout) {
        AbstractFetchTask* t = makeFetchTask("output_file_server");
        Q_ASSERT(t);
        taskLst << t;
        auto ct = static_cast<OutputFileFetchServerTask*>(t);
        Q_ASSERT(ct);
        ct->setFileProvider(provider_);
    }

    for (auto t: taskLst) {
        Q_ASSERT(t);
        t->reset(server,node,fileName,deltaPos, modTime, checkSum, useCache);
        fetchQueue_->add(t);
    }

//#ifdef UI_OUTPUTFILEPROVIDER_DEBUG__
    UiLog().dbg() << UI_FN_INFO << "queue=" << fetchQueue_;
//#endif
    fetchQueue_->run();
}

void OutputFileFetchQueueManager::runMode(VFile::FetchMode fetchMode,
                                         ServerHandler* server, VNode* node,
                                         const std::string& fileName, bool isJobout,
                                         size_t deltaPos, unsigned int modTime, const std::string& checkSum, bool useCache)
{
    fetchQueue_->clear();

    AbstractFetchTask *t=nullptr;
    if (fetchMode == VFile::LogServerFetchMode) {
        t = makeFetchTask("file_logserver");
    } else if(fetchMode == VFile::TransferFetchMode) {
        t = makeFetchTask("file_transfer");
    } else if(fetchMode == VFile::LocalFetchMode) {
        t = makeFetchTask("file_local");
    } else if(isJobout && fetchMode == VFile::ServerFetchMode) {
        t = makeFetchTask("output_file_server");
        auto ct = static_cast<OutputFileFetchServerTask*>(t);
        ct->setFileProvider(provider_);
    }

    if (t) {
        t->reset(server,node,fileName,deltaPos, modTime, checkSum, useCache);
        fetchQueue_->add(t);
    }

    // when remote fetch mode and the file is the current jobout we also try the server as a fallback
    if ((fetchMode == VFile::LogServerFetchMode || fetchMode == VFile::TransferFetchMode) && isJobout) {
        t = makeFetchTask("output_file_server");
        auto ct = static_cast<OutputFileFetchServerTask*>(t);
        ct->setFileProvider(provider_);
        t->reset(server,node,fileName,deltaPos, modTime, checkSum, useCache);
        fetchQueue_->add(t);
    }

//#ifdef UI_OUTPUTFILEPROVIDER_DEBUG__
    UiLog().dbg() << UI_FN_INFO << "fetchMode=" << fetchMode << "queue=" << fetchQueue_;
//#endif
    fetchQueue_->run();
}


void OutputFileFetchQueueManager::fetchQueueSucceeded()
{
    provider_->owner_->infoReady(theReply());
    theReply()->reset();
}

void OutputFileFetchQueueManager::fetchQueueFinished(const std::string& /*filePath*/, VNode* n)
{
    if(n && n->isFlagSet(ecf::Flag::JOBCMD_FAILED))
    {
       theReply()->setErrorText("Submission command failed! Check .sub file, ssh, or queueing system error.");
    }
    if (theReply()->errorText().empty()) {
        theReply()->setErrorText("Failed to fetch file!");
    }
    provider_->owner_->infoFailed(theReply());
    theReply()->reset();
}

void OutputFileFetchQueueManager::progressStart(const std::string& msg, int max)
{
    provider_->owner_->infoProgressStart(msg,max);
}

void OutputFileFetchQueueManager::progressUpdate(const std::string& msg, int value)
{
    provider_->owner_->infoProgressUpdate(msg,value);
}

void OutputFileFetchQueueManager::progressStop()
{
    provider_->owner_->infoProgressStop();
}

VDir_ptr OutputFileFetchQueueManager::dirToFile(const std::string& fileName) const
{
    return provider_->dirToFile(fileName);
}


//========================================
//
// OutputFileProvider
//
//========================================

OutputFileProvider::OutputFileProvider(InfoPresenter* owner) :
    InfoProvider(owner,VTask::OutputTask)
{
    // outCache will be clean up automatically (QObject)
    outCache_=new OutputCache(this);

    fetchManager_ = new OutputFileFetchQueueManager(this);
}

OutputFileProvider::~OutputFileProvider()
{
    if (fetchManager_) {
        delete fetchManager_;
        fetchManager_ = nullptr;
    }
}

// This is called from the destructor
void OutputFileProvider::clear()
{
    //Detach all the outputs registered for this instance in cache
    outCache_->detach();

    // clear the queue and the fetch tasks
    if (fetchManager_) {
        fetchManager_->clear();
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
    // clear the queue
    fetchManager_->clear();

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
    fetchFileInternal(server,n,jobout,true,0,true);
}

void OutputFileProvider::fetchCurrentJobout(bool useCache)
{
    fetchFile(joboutFileName(),0,useCache);
}

//Get a file
void OutputFileProvider::fetchFile(const std::string& fileName, size_t deltaPos, bool useCache)
{
    // clear the queue
    fetchManager_->clear();

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

    fetchFileInternal(server,n,fileName,(fileName==jobout), deltaPos, useCache);
}

void OutputFileProvider::fetchFileInternal(ServerHandler *server,VNode *n,const std::string& fileName,bool isJobout, size_t deltaPos, bool useCache)
{
    fetchManager_->clear();

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
        reply_->addLogMsgEntry("Current job output does not exist yet (<b>TRYNO</b> is <b>0</b>)!");
        owner_->infoReady(reply_);
        return;
    }

    //----------------------------------
    // The host is the localhost
    //----------------------------------

    if(isJobout)
    {
        if(n->isSubmitted())
            reply_->addLogRemarkEntry("This file is the <b>current</b> job output (defined by variable <b>ECF_JOBOUT</b>), but \
                  because the node status is <b>submitted</b> it may contain the output from a previous run!");
        else
            reply_->addLogRemarkEntry("This file is the <b>current</b> job output (defined by variable <b>ECF_JOBOUT</b>).");
    }
    else
    {
        reply_->addLogRemarkEntry("This file is <b>not</b> the <b>current</b> job output (defined by <b>ECF_JOBOUT</b>).");
    }

    fetchManager_->runFull(server,n,fileName,isJobout, deltaPos, 0, {}, useCache);
}

void OutputFileProvider::fetchFileForMode(VFile_ptr f, size_t deltaPos, bool useCache)
{
    if (f) {
        fetchFileForModeInternal(f->sourcePath(), f->fetchMode(), deltaPos, f->sourceModTime(), f->sourceCheckSum(), useCache);
    }
}

//Get a file with the given fetch mode. We use it to fetch files appearing in the directory
//listing in the Output panel.
void OutputFileProvider::fetchFileForModeInternal(const std::string& fileName,VFile::FetchMode fetchMode,size_t deltaPos, unsigned int modTime, const std::string& checkSum, bool useCache)
{
    fetchManager_->clear();

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
        reply_->addLogMsgEntry("Output file is not defined.");
        owner_->infoFailed(reply_);
        return;
    }

    //Check if tryno is 0. ie. the file is the current jobout file and ECF_TRYNO = 0
    if(isTryNoZero(fileName))
    {
        reply_->setInfoText("Current job output does not exist yet (<b>TRYNO</b> is <b>0</b>)!)");
        reply_->addLogMsgEntry("Current job output does not exist yet (<b>TRYNO</b> is <b>0</b>)!");
        owner_->infoReady(reply_);
        return;
    }

    fetchManager_->runMode(fetchMode, server,node,fileName,isJobout, deltaPos, modTime, checkSum, useCache);
}

//Get a file with the given fetch mode. We use it to fetch files appearing in the directory
//listing in the Output panel.
void OutputFileProvider::fetchFileForMode(const std::string& fileName,VDir::FetchMode fetchMode, bool useCache)
{
    VFile::FetchMode fMode =  VFile::NoFetchMode;
    if (fetchMode == VDir::LogServerFetchMode) {
        fMode = VFile::LogServerFetchMode;
    } else if(fetchMode == VDir::TransferFetchMode) {
        fMode = VFile::TransferFetchMode;
    } else if(fetchMode == VDir::LocalFetchMode) {
        fMode = VFile::LocalFetchMode;
    } else if(fetchMode == VDir::ServerFetchMode) {
        fMode = VFile::ServerFetchMode;
    }

    if (fMode != VFile::NoFetchMode) {
        fetchFileForModeInternal(fileName,fMode,0,0, {}, useCache);
    }
}

// this must be called from the queue and must be the last task of the queue.
void OutputFileProvider::fetchJoboutViaServer(ServerHandler *server,VNode *n,const std::string& fileName)
{   
    // From this moment on we do not need the queue and the
    // OutputFileProvider itself will manage the VTask.
    fetchManager_->clear();

    assert(server);
    assert(n);

    //Define a task for getting the info from the server.
    task_=VTask::create(taskType_,n,this);

#ifdef UI_OUTPUTFILEPROVIDER_DEBUG__
    UiLog().dbg() << UI_FN_INFO << "fileName=" << fileName;
#endif
    task_->reply()->fileReadMode(VReply::ServerReadMode);
    task_->reply()->fileName(fileName);
    task_->reply()->setLog(reply_->log());
    task_->reply()->setErrorText(reply_->errorText());

    //Run the task in the server. When it finish taskFinished() is called.
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

static FetchTaskMaker<OutputFileFetchServerTask> maker1("output_file_server");
