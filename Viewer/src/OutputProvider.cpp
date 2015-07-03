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

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/algorithm/string/predicate.hpp>

/*void  InfoProvider::taskChanged(VTask_ptr task)
{
    if(task_ != task)
        return;

    switch(task->status())
    {
        case VTask::FINISHED:
            //We prepend the results to the existing text
            reply_->text(task->reply()->text());
            owner_->infoReady(reply_);
            //We do not need the task anymore.
            task_.reset();
            break;
        case VTask::ABORTED:
        case VTask::CANCELLED:
        case VTask::REJECTED:
            reply_->errorText(task->reply()->errorText());
            owner_->infoFailed(reply_);
            //We do not need the task anymore.
            task_.reset();break;
        default:
            break;
    }
}*/


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


    /*
    reply_->fileName(fileName);

    //Joubout variable is not defined
    if(fileName.empty())
    {
    	reply_->errorText("Variable ECF_JOBOUT is not defined!");
    	owner_->infoFailed(reply_);
    }

    //Check if it is tryno 0
    if(boost::algorithm::ends_with(fileName,".0"))
    {
    	reply_->errorText("No output to be expected when TRYNO is 0!");
    	owner_->infoFailed(reply_);
    	return;
    }

    //Try to use the logserver to fetch the file
    else if(fetchFileViaLogServer(n,fileName))
    {
    	owner_->infoReady(reply_);
    	return;
    }

    //We try to read the file directly from the disk
    else if(info->server())
    {
    	//We try to read the file directly from the disk
    	if(info->server()->readFromDisk())
    	{
    		//Get the fileName
    		if(reply_->textFromFile(fileName))
    		{
    			reply_->fileReadMode(VReply::LocalReadMode);
    			owner_->infoReady(reply_);
    			return;
    		}
    	}
    	else
    	{
    		reply_->fileReadMode(VReply::ServerReadMode);

    		//Define a task for getting the info from the server.
    		task_=VTask::create(taskType_,info->node(),this);

    		//Run the task in the server. When it finish taskFinished() is called. The text returned
    		//in the reply will be prepended to the string we generated above.
    		info->server()->run(task_);
    		return;
    	}
    }
    else
    {
    	reply_->errorText("No server found!!");
    	owner_->infoFailed(reply_);
    }*/
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
	if(!n || !n->node())
    {
    	owner_->infoFailed(reply_);
    	return;
    }

	//Set the filename in reply
	reply_->fileName(fileName);

    //Joubout variable is not defined
    if(isJobout && fileName.empty())
    {
    	reply_->errorText("Variable ECF_JOBOUT is not defined!");
    	owner_->infoFailed(reply_);
    }

    //Check if it is tryno 0
    if(boost::algorithm::ends_with(fileName,".0"))
    {
    	reply_->errorText("No output to be expected when TRYNO is 0!");
    	owner_->infoFailed(reply_);
    	return;
    }

    //Try to use the logserver to fetch the file
    else if(fetchFileViaLogServer(n,fileName))
    {
    	owner_->infoReady(reply_);
    	return;
    }

    //We try to read the file directly from the disk
    else if(server)
    {
    	//We try to read the file directly from the disk
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
    	else
    	{
    		reply_->fileReadMode(VReply::ServerReadMode);

    		//Define a task for getting the info from the server.
    		task_=VTask::create(taskType_,n,this);

    		//Run the task in the server. When it finish taskFinished() is called. The text returned
    		//in the reply will be prepended to the string we generated above.
    		server->run(task_);
    		return;
    	}
    }
    else
    {
    	reply_->errorText("No server found!!");
    	owner_->infoFailed(reply_);
    	return;
    }

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

	//it is a directory
	if(boost::filesystem::is_directory(p))
	{
		return res;
	}

	//It is a file
	else
	{
		if(boost::filesystem::exists(p.parent_path()))
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

	return res;
}




