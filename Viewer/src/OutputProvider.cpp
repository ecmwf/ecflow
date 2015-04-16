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
	reply_->reset();

    if(!info->node() || !info->node()->node())
	{
    	owner_->infoFailed(reply_);
	}

    VNode *n=info->node();

    //Get the filename
    std::string fileName=n->genVariable("ECF_JOBOUT");

    reply_->fileName(fileName);

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
    	reply_->fileReadMode(VReply::LogServerReadMode);
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
    }
}

bool OutputProvider::fetchFileViaLogServer(VNode *n,const std::string& fileName)
{
	std::string logHost=n->genVariable("ECF_LOGHOST");
	std::string logPort=n->genVariable("ECF_LOGPORT");
	if(logHost.empty())
	{
		logHost=n->genVariable("LOGHOST");
		logPort=n->genVariable("LOGPORT");
	}

	std::string::size_type pos = logHost.find(n->genVariable("ECF_MICRO"));
	if(std::string::npos == pos && !logHost.empty())
	{
		//Create a logserver
		LogServer logServer(logHost,logPort);
		if(logServer.ok())
		{
			VFile_ptr tmp = logServer.getFile(fileName);
			if(tmp->exists())
			{
				//reply_->tmpFile(tmp);
				//reply_->directory(logServer.getDir(fileName));
				return true;
			}
		}
	}
	return false;
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

	//Try local dir
	dir=fetchLocalDir(fileName);
	if(dir)
		return dir;
	else
		dir=fetchDirViaLogServer(n,fileName);

	return dir;
}


VDir_ptr OutputProvider::fetchDirViaLogServer(VNode *n,const std::string& fileName)
{
	VDir_ptr res;

	std::string logHost=n->genVariable("ECF_LOGHOST");
	std::string logPort=n->genVariable("ECF_LOGPORT");
	if(logHost.empty())
	{
		logHost=n->genVariable("LOGHOST");
		logPort=n->genVariable("LOGPORT");
	}

	std::string::size_type pos = logHost.find(n->genVariable("ECF_MICRO"));
	if(std::string::npos == pos && !logHost.empty())
	{
		//Create a logserver
		LogServer logServer(logHost,logPort);
		if(logServer.ok())
		{
			VFile_ptr tmp = logServer.getFile(fileName);
			if(tmp->exists())
			{
				res=logServer.getDir(fileName.c_str());
				return res;

			}
		}
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




