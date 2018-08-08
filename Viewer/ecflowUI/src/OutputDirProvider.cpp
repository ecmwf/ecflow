//============================================================================
// Copyright 2009-2017 ECMWF.
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

OutputDirProvider::OutputDirProvider(InfoPresenter* owner) :
	InfoProvider(owner,VTask::NoTask),
    outClient_(nullptr),
    currentTask_(-1)
{
}

void OutputDirProvider::clear()
{
	if(outClient_)
	{
		delete outClient_;
		outClient_=nullptr;
	}

    queue_.clear();
    currentTask_=-1;
	InfoProvider::clear();
}

//Node
void OutputDirProvider::visit(VInfoNode* info)
{
	//Reset the reply
	reply_->reset();

    //Clear the queue
    queue_.clear();
    currentTask_=-1;

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
    queue_ << OutputDirProviderTask(joboutFile,OutputDirProviderTask::RemoteFetch);

    //With the readFromdisk option we always try read locally
    if(server->readFromDisk())
    {
        queue_ << OutputDirProviderTask(joboutFile,OutputDirProviderTask::LocalFetch);
    }
    //Otherwise we only read the local disk if the remote access (ie logserver) failed
    else
    {
        queue_ << OutputDirProviderTask(joboutFile,OutputDirProviderTask::LocalFetch,
                                        OutputDirProviderTask::RunIfPrevFailed);
    }

    std::string outFile = n->findVariable("ECF_OUT",true);
    std::string jobFile = n->findVariable("ECF_JOB",true);
    if(!outFile.empty() && !jobFile.empty())
    {
        queue_ << OutputDirProviderTask(jobFile,OutputDirProviderTask::RemoteFetch);

        //With the readFromdisk option we always try read locally
        if(server->readFromDisk())
        {
            queue_ << OutputDirProviderTask(jobFile,OutputDirProviderTask::LocalFetch);
        }
        //Otherwise we only read the local disk if the remote access (ie logserver) failed
        else
        {
            queue_ << OutputDirProviderTask(jobFile,OutputDirProviderTask::LocalFetch,
                                            OutputDirProviderTask::RunIfPrevFailed);
        }
    }

    //Start fetching the dirs
    fetchNext();
}

void OutputDirProvider::fetchIgnored()
{
    if(hasNext())
        fetchNext();
    else
        completed();
}

void OutputDirProvider::fetchFinished(VDir_ptr dir,QString msg)
{
    Q_ASSERT(currentTask_ >=0  && currentTask_ < queue_.count());
    OutputDirProviderTask task=queue_[currentTask_];
    queue_[currentTask_].dir_=dir;
    queue_[currentTask_].status_=OutputDirProviderTask::FinishedStatus;

    if(hasNext())
        fetchNext();
    else
        completed();
}

void OutputDirProvider::fetchFailed(QString msg)
{
    Q_ASSERT(currentTask_ >=0  && currentTask_ < queue_.count());
    OutputDirProviderTask task=queue_[currentTask_];
    queue_[currentTask_].status_=OutputDirProviderTask::FailedStatus;
    queue_[currentTask_].error_=msg;

    if(hasNext())
        fetchNext();
    else
        completed();
}

void OutputDirProvider::failed(QString msg)
{
    queue_.clear();
    currentTask_=-1;

    reply_->setInfoText("");
    reply_->setErrorText(msg.toStdString());
    owner_->infoFailed(reply_);
}

void OutputDirProvider::completed()
{
    bool hasAllFailed=true;
    std::vector<VDir_ptr> dirVec;
    Q_FOREACH(OutputDirProviderTask task,queue_)
    {
        if(task.dir_)
            dirVec.push_back(task.dir_);

        if(!task.error_.isEmpty())
            reply_->appendErrorText(task.error_.toStdString());

        if(task.status_ != OutputDirProviderTask::FailedStatus)
            hasAllFailed=false;
    }

    //clear the queue
    queue_.clear();
    currentTask_=-1;

    //Notify owner
    reply_->setDirectories(dirVec);
    owner_->infoReady(reply_);

    reply_->setInfoText("");
    reply_->setDirectories(dirVec);
    if(!hasAllFailed)
        owner_->infoReady(reply_);
    else
        owner_->infoFailed(reply_);
}

bool OutputDirProvider::hasNext() const
{
    return currentTask_ < queue_.count()-1;
}

void OutputDirProvider::fetchNext()
{
    //point to the next task
    currentTask_++;

    Q_ASSERT(currentTask_ >=0 && currentTask_ < queue_.count());
    if(currentTask_ >= queue_.count())
        return completed();

    VDir_ptr dir;
    if(!info_ || !info_->isNode() || !info_->node() || !info_->node()->node())
    {
        failed("Node not found"); //fatal error
        return;
    }

    ServerHandler* server=info_->server();
    VNode *n=info_->node();
    if(!server || !n || !n->node())
    {
        failed("Node not found"); //fatal error
        return;
    }

    //Get the current task
    OutputDirProviderTask task=queue_[currentTask_];
    //Check conditions
    if(currentTask_ > 0)
    {
        OutputDirProviderTask prevTask=queue_[currentTask_-1];
        if(task.condition_ == OutputDirProviderTask::RunIfPrevFailed &&
           prevTask.status_ !=  OutputDirProviderTask::FailedStatus)
        {
            fetchIgnored(); //we simply skip this task
            return;
        }

    }

    //Get the file name
    std::string fileName=task.path_;

    //Jobout is empty: no dir path is availabale
    if(fileName.empty())
    {
        fetchFailed("Directory path is empty");
        return;
    }

    //We try the output client, its asynchronous! Here we create an
    //output client and ask to the fetch the dir asynchronously. The ouput client will call
    //slotOutputClientFinished() or slotOutputClientError() eventually!!
    if(task.fetchMode_ ==  OutputDirProviderTask::RemoteFetch)
    {
        if(!fetchDirViaOutputClient(n,fileName))
        {
            //no logserver is defined
            fetchIgnored();
        }
        return;
    }

    //We try to read it from disk
    if(task.fetchMode_ == OutputDirProviderTask::LocalFetch)
    {
        //If there is no output client we try to read it from disk
        std::string errStr;
        dir=fetchLocalDir(fileName,errStr);
        if(dir)
        {
            fetchFinished(dir);
        }
        else
        {
            fetchFailed(QString::fromStdString(errStr));
        }
        return;
    }

    //If we are here the error or warning is already set in reply
    fetchFailed();
}

bool OutputDirProvider::fetchDirViaOutputClient(VNode *n,const std::string& fileName)
{
    std::string host, port;
    if(n->logServer(host,port))
    {
        outClient_=makeOutputClient(host,port);
        outClient_->getDir(fileName);
        return true;
    }

    return false;
}

void OutputDirProvider::slotOutputClientFinished()
{
    if(!info_ || queue_.isEmpty())
        return;

    Q_ASSERT(outClient_);
    VDir_ptr dir = outClient_->result();
    if(dir)
    {
        dir->setFetchMode(VDir::LogServerFetchMode);
        std::string method="served by " + outClient_->host() + "@" + outClient_->portStr();
        dir->setFetchModeStr(method);
        dir->setFetchDate(QDateTime::currentDateTime());
        fetchFinished(dir);
        return;
    }

    fetchFailed();
}

void OutputDirProvider::slotOutputClientProgress(QString,int)
{
}

void OutputDirProvider::slotOutputClientError(QString msg)
{
    if(!info_ || queue_.isEmpty())
        return;

    QString sDesc;
    if(outClient_)
    {
        sDesc="Failed to fetch from " + QString::fromStdString(outClient_->host()) +
                "@" + QString::fromStdString(outClient_->portStr());
        if(!msg.isEmpty())
            sDesc+=" error: " + msg;

    }
    else
    {
        sDesc="Failed to fetch from logserver";
        if(!msg.isEmpty())
            sDesc+=": " + msg;
    }

    fetchFailed(sDesc);
}

VDir_ptr OutputDirProvider::fetchLocalDir(const std::string& path,std::string& errorStr)
{
    VDir_ptr res;

    boost::filesystem::path p(path);

    //Is it a directory?
    boost::system::error_code errorCode;
    if(boost::filesystem::is_directory(p,errorCode))
    {
        return res;
    }

    try
    {
        if(boost::filesystem::exists(p.parent_path()))
        {
            std::string dirName=p.parent_path().string();
            if(info_ && info_->isNode() && info_->node())
            {
                std::string nodeName=info_->node()->strName();
                std::string pattern=nodeName+".";
                res=std::make_shared<VDir>(dirName,pattern);
                res->setFetchDate(QDateTime::currentDateTime());
                res->setFetchMode(VDir::LocalFetchMode);
                res->setFetchModeStr("from disk");
                return res;
            }
        }

        errorStr="No access to path on disk!";
        return res;
    }
    catch (const boost::filesystem::filesystem_error& e)
    {
        errorStr="No access to path on disk! error: " + std::string(e.what());
        UiLog().warn() << "fetchLocalDir failed:" << std::string(e.what());
    }

    return res;
}

OutputDirClient* OutputDirProvider::makeOutputClient(const std::string& host,const std::string& port)
{
    if(outClient_)
    {
        if(outClient_->host() != host || outClient_->portStr() != port)
        {
            delete outClient_;
            outClient_=nullptr;
        }
    }

    if(!outClient_)
    {
        outClient_=new OutputDirClient(host,port,this);

        connect(outClient_,SIGNAL(error(QString)),
                this,SLOT(slotOutputClientError(QString)));

        connect(outClient_,SIGNAL(progress(QString,int)),
                this,SLOT(slotOutputClientProgress(QString,int)));

        connect(outClient_,SIGNAL(finished()),
                this,SLOT(slotOutputClientFinished()));
    }

    return outClient_;
}
