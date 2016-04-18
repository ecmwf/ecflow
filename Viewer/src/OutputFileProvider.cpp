//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "OutputFileProvider.hpp"

#include "LogServer.hpp"
#include "OutputFileClient.hpp"
#include "VNode.hpp"
#include "VReply.hpp"
#include "ServerHandler.hpp"
#include "UserMessage.hpp"

#include <QDateTime>

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/algorithm/string/predicate.hpp>

OutputFileProvider::OutputFileProvider(InfoPresenter* owner) :
	InfoProvider(owner,VTask::OutputTask),
    outClient_(NULL), latestCached_(NULL)
{
}

void OutputFileProvider::clear()
{
    OutputCache::instance()->detach(latestCached_);
    latestCached_=NULL;

    if(outClient_)
	{
		delete outClient_;
		outClient_=NULL;
	}
    InfoProvider::clear();

    dir_.reset();
}

//Node
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
    std::string jobout=joboutFileName(); //n->findVariable("ECF_JOBOUT",true);

    //This is needed for the refresh!!! We want to detach the item but keep
    //lastCached.
    bool detachCache=!(latestCached_ && latestCached_->sameAs(info_,jobout));
    if(!detachCache)
    {
        OutputCache::instance()->detach(latestCached_);
    }

    fetchFile(server,n,jobout,true,detachCache);
}

//Get a file
void OutputFileProvider::file(const std::string& fileName)
{
    //When a new file is requested
    OutputCache::instance()->detach(latestCached_);
    latestCached_=NULL;

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

    fetchFile(server,n,fileName,(fileName==jobout),false);
}

void OutputFileProvider::fetchFile(ServerHandler *server,VNode *n,const std::string& fileName,bool isJobout,bool detachCache)
{
    if(detachCache)
    {
        OutputCache::instance()->detach(latestCached_);
        latestCached_=NULL;
    }

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

    //Check if it is tryno 0
    if(boost::algorithm::ends_with(fileName,".0"))
    {
        reply_->setInfoText("Current job output does not exist yet (<b>TRYNO</b> is <b>0</b>)!)");
        reply_->addLog("MSG>Current job output does not exist yet (<b>TRYNO</b> is <b>0</b>)!");
        owner_->infoReady(reply_);
    	return;
    }

    if(isJobout && n->isSubmitted())
    {
        reply_->setInfoText("Current job output does not exist yet (node status is <b>submitted</b>!)");
        reply_->addLog("MSG>Current job output does not exist yet (node status is <b>submitted</b>!)");
        owner_->infoReady(reply_);
        return;
    }

    //----------------------------------
    // The host is the localhost
    //----------------------------------

    if(isJobout)
       reply_->addLog("REMARK>This file is the <b>current</b> job output (defined by variable <b>ECF_JOBOUT</b>).");
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

    //----------------------------------------------------
    // Not the loacalhost or we could not read the file
    //----------------------------------------------------

    //We try the output client, its asynchronous!
    if(fetchFileViaOutputClient(n,fileName))
    {
    	//If we are here we created a output client and asked to the fetch the
    	//file asynchronously. The ouput client will call slotOutputClientFinished() or
    	//slotOutputClientError eventually!!
    	return;
    }

    reply_->addLog("TRY>fetch file from logserver: NOT DEFINED");

    //If there is no output client we try
    //to read it again from the disk!!!  
    if(server->readFromDisk() || !isJobout)
    {
        //Get the fileName
        if(fetchLocalFile(fileName))
            return;
    }

    //If we are here no output client is defined and we could not read the file from
    //the local disk.
    //We try the server if it is the jobout file
    if(isJobout)
    {
    	fetchJoboutViaServer(server,n,fileName);
        return;
    }

    //If we are here we coud not get the file
    owner_->infoFailed(reply_);
}

bool OutputFileProvider::fetchFileViaOutputClient(VNode *n,const std::string& fileName)
{
    std::string host, port;
    assert(n);

    UserMessage::debug("OutputFileProvider::fetchFileViaOutputClient <-- file: " + fileName);

    //If it is not the jobout file or it is the joubout but it is not the current item in the cache
    //(i.e. we do not want to referesh it) we try to use the cache
    if(fileName != joboutFileName() || !(latestCached_))
    {
        //Check cache
        if(OutputCacheItem* item=OutputCache::instance()->use(info_,fileName))
        {
            latestCached_=item;
            VFile_ptr f=item->file();
            assert(f);
            f->setCached(true);

            UserMessage::debug("  File found in cache");

            reply_->setInfoText("");
            reply_->fileReadMode(VReply::LogServerReadMode);

            reply_->setLog(f->log());
            reply_->addLog("REMARK>File was read from cache.");

            reply_->tmpFile(f);
            owner_->infoReady(reply_);
            return true;
        }
    }

    //If we are here we do not need to know the previously cached item
    latestCached_=NULL;

    //We did not used the cache
    if(n->logServer(host,port))
	{
		//host=host + "baaad";

        UserMessage::debug("OutputFileProvider::fetchFileViaOutputClient --> host:" + host +
                             " port:" + port + " file: " + fileName);

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

            outClient_->setDir(dir_);
		}

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

    latestCached_=OutputCache::instance()->add(info_,tmp->sourcePath(),tmp);

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
    //UserMessage::debug("OutputFileProvider::slotOutputClientProgress " + msg.toStdString());

    owner_->infoProgress(msg.toStdString(),value);

    //reply_->setInfoText(msg.toStdString());
    //owner_->infoProgress(reply_);
    //reply_->setInfoText("");

    //qDebug() << "prog: " << msg;
}



void OutputFileProvider::slotOutputClientError(QString msg)
{
    UserMessage::message(UserMessage::DBG,false,"OutputFileProvider::slotOutputClientError error:" + msg.toStdString());
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
            if(server->readFromDisk())
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

void OutputFileProvider::setDir(VDir_ptr dir)
{
    if(outClient_)
        outClient_->setDir(dir);  

    if(dir != dir_)
        dir_=dir;
}

