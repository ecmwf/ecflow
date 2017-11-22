//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "OutputFileProvider.hpp"

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

OutputFileProvider::OutputFileProvider(InfoPresenter* owner) :
	InfoProvider(owner,VTask::OutputTask),
    outClient_(NULL)
{
    outCache_=new OutputCache(this);
}

void OutputFileProvider::clear()
{
    //Detach all the outputs registered for this instance in cache
    outCache_->detach();

    if(outClient_)
	{
		delete outClient_;
		outClient_=NULL;
	}
    InfoProvider::clear();

    dirs_.clear();
}

//This is called when we load a new node in the Output panel. In this
//case we always try to load the current jobout file.
void OutputFileProvider::visit(VInfoNode* infoNode)
{
    assert(info_->node() == infoNode->node());

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
    fetchFile(server,n,jobout,true,true);
}

//Get a file
void OutputFileProvider::file(const std::string& fileName,bool useCache)
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
	VNode *n=info_->node();

    //Get the filename
    std::string jobout=joboutFileName(); //n->findVariable("ECF_JOBOUT",true);

    fetchFile(server,n,fileName,(fileName==jobout),useCache);
}

void OutputFileProvider::fetchFile(ServerHandler *server,VNode *n,const std::string& fileName,bool isJobout,bool useCache)
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
                  because the node status is <b>submitted</b> it may contain the ouput from a previous run!");
        else
            reply_->addLog("REMARK>This file is the <b>current</b> job output (defined by variable <b>ECF_JOBOUT</b>).");
    }
    else
       reply_->addLog("REMARK>This file is <b>not</b> the <b>current</b> job output (defined by <b>ECF_JOBOUT</b>).");

#if 0
    if(server->readFromDisk())
    {
        if(server->isLocalHost())
        {
        //We try to read the file directly from the disk
        if(server->readFromDisk())
        {
            if(fetchLocalFile(fileName))
                return;
        }
    }
#endif

#if 0
    //if(server->isLocalHost())
    // {
    	//We try to read the file directly from the disk
        if(server->readFromDisk())
    	{
    		if(fetchLocalFile(fileName))
    			return;
    	}
    //}

#endif


    //We try the output client (aka logserver), its asynchronous!
    if(fetchFileViaOutputClient(n,fileName,useCache))
    {
        //If we are here we created an output client and asked it to the fetch the
    	//file asynchronously. The ouput client will call slotOutputClientFinished() or
    	//slotOutputClientError eventually!!
    	return;
    }

    //If we are here there is no outpt client defined
    reply_->addLog("TRY>fetch file from logserver: NOT DEFINED");

    //If there is no output client we try
    //to read it from the disk (if the settings allow it)
    if(server->readFromDisk() || !isJobout)
    {
        //Get the fileName
        if(fetchLocalFile(fileName))
            return;
    }

    //If we are here no output client is defined and we could not read the file from
    //the local disk, so we try the server if it is the jobout file.
    if(isJobout)
    {
    	fetchJoboutViaServer(server,n,fileName);
        return;
    }

    //If we are here we coud not get the file
    if(n->isFlagSet(ecf::Flag::JOBCMD_FAILED))
    {
        reply_->setErrorText("Submission command failed! Check .sub file, ssh, or queueing system error.");
    }
    owner_->infoFailed(reply_);
}

bool OutputFileProvider::fetchFileViaOutputClient(VNode *n,const std::string& fileName,bool useCache)
{
    UI_FUNCTION_LOG
    UiLog().dbg() << "OutputFileProvider::fetchFileViaOutputClient <-- file: " << fileName;

    std::string host, port;
    assert(n);

    //We try use the cache
    if(useCache)
    {
        //Check if the given output is already in the cache
        if(OutputCacheItem* item=outCache_->attachOne(info_,fileName))
        {           
            VFile_ptr f=item->file();
            assert(f);
            f->setCached(true);

            UiLog().dbg() << "  File found in cache";

            reply_->setInfoText("");
            reply_->fileReadMode(VReply::LogServerReadMode);

            reply_->setLog(f->log());
            reply_->addLog("REMARK>File was read from cache.");

            reply_->tmpFile(f);
            owner_->infoReady(reply_);
            return true;
        }
    }

    //We did/could not use the cache
    if(n->logServer(host,port))
	{
		//host=host + "baaad";

        UiLog().dbg() << "OutputFileProvider::fetchFileViaOutputClient --> host:" << host <<
                             " port:" << port << " file: " << fileName;

        //reply_->setInfoText("Getting file through log server: " + host + "@" + port);
        //owner_->infoProgress(reply_);
        owner_->infoProgressStart("Getting file <i>" + fileName + "</i> from log server <i>" + host + "@" + port  +"</i>",0);

		if(!outClient_)
		{
			outClient_=new OutputFileClient(host,port,this);

			connect(outClient_,SIGNAL(error(QString)),
				this,SLOT(slotOutputClientError(QString)));

            connect(outClient_,SIGNAL(progress(QString,int)),
                this,SLOT(slotOutputClientProgress(QString,int)));

			connect(outClient_,SIGNAL(finished()),
				this,SLOT(slotOutputClientFinished()));
		}

        VDir_ptr dir=dirToFile(fileName);
        outClient_->setDir(dir);
        outClient_->getFile(fileName);

		return true;
	}

	return false;
}

void OutputFileProvider::slotOutputClientFinished()
{
	VFile_ptr tmp = outClient_->result();
    assert(tmp);

    outClient_->clearResult();

    //Files retrieved from the log server are automatically added to the cache!
    outCache_->add(info_,tmp->sourcePath(),tmp);

    reply_->setInfoText("");
    reply_->fileReadMode(VReply::LogServerReadMode);
    reply_->addLog("TRY> fetch file from logserver: " + outClient_->host() + "@" + outClient_->portStr() + ": OK");

    tmp->setFetchMode(VFile::LogServerFetchMode);
    tmp->setLog(reply_->log());
    std::string method="served by " + outClient_->host() + "@" + outClient_->portStr();
    tmp->setFetchModeStr(method);

    reply_->tmpFile(tmp);
    owner_->infoReady(reply_);
}

void OutputFileProvider::slotOutputClientProgress(QString msg,int value)
{
    //UiLog().dbg() << "OutputFileProvider::slotOutputClientProgress " << msg;

    owner_->infoProgress(msg.toStdString(),value);

    //reply_->setInfoText(msg.toStdString());
    //owner_->infoProgress(reply_);
    //reply_->setInfoText("");
}

void OutputFileProvider::slotOutputClientError(QString msg)
{
    UiLog().dbg() << "OutputFileProvider::slotOutputClientError error:" << msg;
    reply_->addLog("TRY> fetch file from logserver: " + outClient_->host() + "@" + outClient_->portStr() + " FAILED");

    if(info_)
	{
		ServerHandler* server=info_->server();
		VNode *n=info_->node();

		if(server && n)
		{
			std::string jobout=n->findVariable("ECF_JOBOUT",true);
            bool isJobout=(outClient_->remoteFile() == jobout);

            //We try to read the file directly from the disk
            if(server->readFromDisk() || !isJobout)
            {
                if(fetchLocalFile(outClient_->remoteFile()))
                    return;
            }

            //Then we try the server
            if(isJobout)
			{
				fetchJoboutViaServer(server,n,jobout);
				return;
			}
		}
	}

    reply_->setErrorText("Failed to fetch file from logserver "  +
                         outClient_->host() + "@" + outClient_->portStr() +
                         "Error: " + msg.toStdString());
	owner_->infoFailed(reply_);
}

void OutputFileProvider::fetchJoboutViaServer(ServerHandler *server,VNode *n,const std::string& fileName)
{
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

bool OutputFileProvider::fetchLocalFile(const std::string& fileName)
{
	//we do not want to delete the file once the VFile object is destroyed!!
	VFile_ptr f(VFile::create(fileName,false));
	if(f->exists())
	{
        reply_->fileReadMode(VReply::LocalReadMode);
        reply_->addLog("TRY> read file from disk: OK");

        f->setSourcePath(f->path());
        f->setFetchMode(VFile::LocalFetchMode);
        f->setFetchDate(QDateTime::currentDateTime());
        f->setLog(reply_->log());

        reply_->tmpFile(f);
		owner_->infoReady(reply_);
		return true;
	}
    reply_->addLog("TRY> read file from disk: NO ACCESS");
	return false;
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

    for(std::size_t i=0; i < dirs_.size(); i++)
    {
        if(dirs_[i] && fileName.find(dirs_[i]->path()) == 0)
            return dirs_[i];

    }
    return dir;
}
