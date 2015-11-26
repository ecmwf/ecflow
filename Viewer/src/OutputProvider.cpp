//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "OutputProvider.hpp"

#include "LogServer.hpp"
#include "VNode.hpp"
#include "VReply.hpp"
#include "ServerHandler.hpp"
#include "UserMessage.hpp"

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/algorithm/string/predicate.hpp>

//Node
void OutputProvider::visit(VInfoNode* info)
{
	//Reset the reply
	reply_->reset();

	if(!info)
 	{
       	owner_->infoFailed(reply_);
   	}

	ServerHandler* server=info_->server();
	VNode *n=info->node();

    if(!n || !n->node())
   	{
       	owner_->infoFailed(reply_);
   	}

    //Get the filename
    std::string fileName=n->findVariable("ECF_JOBOUT",true);

    fetchFile(server,n,fileName,true);
}

//Get a file
void OutputProvider::file(const std::string& fileName)
{
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
	}

	ServerHandler* server=info_->server();
	VNode *n=info_->node();

	//Get the filename
	std::string jobout=n->findVariable("ECF_JOBOUT",true);

	fetchFile(server,n,fileName,(fileName==jobout));
}

void OutputProvider::fetchFile(ServerHandler *server,VNode *n,const std::string& fileName,bool isJobout)
{
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
			owner_->infoFailed(reply_);
		}
		else
		{
			reply_->setErrorText("Output file is not defined!");
			owner_->infoFailed(reply_);
		}

		return;
	}

    //Check if it is tryno 0
    if(boost::algorithm::ends_with(fileName,".0"))
    {
    	reply_->setInfoText("No output to be expected when TRYNO is 0!");
    	owner_->infoReady(reply_);
    	return;
    }

    //The host is the localhost
    if(server->isLocalHost())
    {
    	//First we try to read the file directly from the disk
    	if(!isJobout || server->readFromDisk())
    	{
    	    //Get the fileName
    	    if(reply_->textFromFile(fileName))
    	    {
    	    	reply_->fileReadMode(VReply::LocalReadMode);
    	    	owner_->infoReady(reply_);
    	    	return;
    	    }
    	}

    	//Then we try the logserver
    	if(fetchFileViaLogServer(n,fileName))
    	{
    		owner_->infoReady(reply_);
    	    return;
    	}
    }
    //The host is another machine
    else
    {
    	//First we try to use the logserver to fetch the file
    	if(fetchFileViaLogServer(n,fileName))
    	{
    		owner_->infoReady(reply_);
    		return;
    	}

    	//Then we try to read the file directly from the disk
    	if(!isJobout || server->readFromDisk())
    	{
    		//Get the fileName
    		if(reply_->textFromFile(fileName))
    		{
    			reply_->fileReadMode(VReply::LocalReadMode);
    			owner_->infoReady(reply_);
    			return;
    		}
    	}
    }

    //Finally we try the server if it is the jobout file
    if(isJobout)
    {
    	reply_->fileReadMode(VReply::ServerReadMode);

        //Define a task for getting the info from the server.
        task_=VTask::create(taskType_,n,this);

        //Run the task in the server. When it finish taskFinished() is called. The text returned
        //in the reply will be prepended to the string we generated above.
        server->run(task_);
        return;
    }

    //If we are we coud not get the file
    owner_->infoFailed(reply_);
}

VDir_ptr OutputProvider::directory()
{
	VDir_ptr dir;

	if(!info_ || !info_->isNode() || !info_->node() || !info_->node()->node())
	{
	    return dir;
	}

	VNode *n=info_->node();

	//Get the filename
	std::string fileName=n->genVariable("ECF_JOBOUT");

	if(fileName.empty())
		return dir;

	//Try local dir
	dir=fetchLocalDir(fileName);
	if(dir)
		return dir;
	//Then the logserver
	else
		dir=fetchDirViaLogServer(n,fileName);

	return dir;
}


bool OutputProvider::fetchFileViaLogServer(VNode *n,const std::string& fileName)
{
	//Create a logserver
	LogServer_ptr logServer=n->logServer();

	if(logServer && logServer->ok())
	{
		VFile_ptr tmp = logServer->getFile(fileName);

		if(tmp && tmp.get() && tmp->exists())
		{
			reply_->fileReadMode(VReply::LogServerReadMode);

			std::string method="served by " + logServer->host() + "@" + logServer->port();
			reply_->fileReadMethod(method);

			reply_->tmpFile(tmp);

			return true;
		}
	}

	return false;

}

VDir_ptr OutputProvider::fetchDirViaLogServer(VNode *n,const std::string& fileName)
{
	VDir_ptr res;

	//Create a logserver
	LogServer_ptr logServer=n->logServer();

	if(logServer && logServer->ok())
	{
		res=logServer->getDir(fileName.c_str());
	}

	return res;
}

VDir_ptr OutputProvider::fetchLocalDir(const std::string& path)
{
	VDir_ptr res;

	boost::filesystem::path p(path);

	try {
		//Is it a directory?
		if(boost::filesystem::is_directory(p))
		{
			return res;
		}
		//It must be a file
		if(boost::filesystem::exists(p) &&
		   boost::filesystem::exists(p.parent_path()))
		{
			std::string dirName=p.parent_path().string();
			std::string fileName=p.leaf().string();

			std::string::size_type pos=fileName.find_last_of(".");
			if(pos != std::string::npos)
			{
				std::string pattern=fileName.substr(0,pos);
				res=VDir_ptr(new VDir(dirName,pattern));
				return res;
			}
		}

	}
	catch (const boost::filesystem::filesystem_error& e)
	{
		UserMessage::message(UserMessage::WARN,false,"fetchLocalDir failed:" + std::string(e.what()));
		return res;
	}


	return res;
}


std::string OutputProvider::joboutFileName() const
{
	if(info_ && info_->isNode() && info_->node() && info_->node()->node())
	{
		return info_->node()->findVariable("ECF_JOBOUT",true);
	}

	return std::string();
}



