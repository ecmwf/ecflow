//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "OutputDirProvider.hpp"

#include "LogServer.hpp"
#include "OutputDirClient.hpp"
#include "VNode.hpp"
#include "VReply.hpp"
#include "ServerHandler.hpp"
#include "UserMessage.hpp"

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/algorithm/string/predicate.hpp>

OutputDirProvider::OutputDirProvider(InfoPresenter* owner) :
	InfoProvider(owner,VTask::NoTask),
	outClient_(NULL)
{

}

void OutputDirProvider::clear()
{
	if(outClient_)
	{
		delete outClient_;
		outClient_=NULL;
	}
	InfoProvider::clear();
}

//Node
void OutputDirProvider::visit(VInfoNode* info)
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

    fetchDir(server,n);
}

void OutputDirProvider::fetchDir(ServerHandler* server,VNode* n)
{
	VDir_ptr dir;

	if(!info_ || !info_->isNode() || !info_->node() || !info_->node()->node())
	{
		reply_->setDirectory(dir);
		owner_->infoFailed(reply_);
	}

	//Get the jobout name
	std::string fileName=n->findVariable("ECF_JOBOUT",true);

	//Check if it is tryno 0
	bool tynozero=(boost::algorithm::ends_with(fileName,".0"));

	//Jobout is empty: no dir path is availabale
	if(fileName.empty())
	{
		reply_->setDirectory(dir);
		owner_->infoFailed(reply_);

		return;
	}

    //----------------------------------
    // The host is the localhost
    //----------------------------------

    if(server->isLocalHost())
    {
    	dir=fetchLocalDir(fileName,tynozero);
    	if(dir)
    	{
    		reply_->setDirectory(dir);
    		owner_->infoReady(reply_);
    		return;
    	}
    }

    //----------------------------------------------------
    // Not the localhost or we could not read dir
    //----------------------------------------------------

    //We try the output client, its asynchronous!
    if(fetchDirViaOutputClient(n,fileName))
    {
      	//If we are here we created a output client and asked to the fetch the
      	//file asynchronously. The ouput client will call slotOutputClientFinished() or
      	//slotOutputClientError eventually!!
      	return;
    }

    //If there is no output client and it is not the localhost we try
    //to read it again from the disk!!!
     if(!server->isLocalHost())
     {
    	 dir=fetchLocalDir(fileName,tynozero);
    	 if(dir)
    	 {
    	     reply_->setDirectory(dir);
    	     owner_->infoReady(reply_);
    	     return;
    	 }
     }

     //If we are we coud not get the file
     reply_->setDirectory(dir);
     owner_->infoFailed(reply_);
}


bool OutputDirProvider::fetchDirViaOutputClient(VNode *n,const std::string& fileName)
{
	std::string host, port;
	if(n->logServer(host,port))
	{
		//host=host + "baaad";

		//reply_->setInfoText("Getting file through log server: " + host + "@" + port);
		//owner_->infoProgress(reply_);

		outClient_=makeOutputClient(host,port);
		outClient_->getDir(fileName);

		return true;
	}

	return false;
}

void OutputDirProvider::slotOutputClientFinished()
{
	VDir_ptr dir = outClient_->result();

	if(dir && dir.get())
	{
		reply_->setInfoText("");
		//reply_->fileReadMode(VReply::LogServerReadMode);

		//std::string method="served by " + outClient_->host() + "@" + outClient_->portStr();
		//reply_->fileReadMethod(method);

		reply_->setDirectory(dir);
		owner_->infoReady(reply_);
	}
}

void OutputDirProvider::slotOutputClientProgress(QString msg)
{
	/*reply_->setInfoText(msg.toStdString());
	owner_->infoProgress(reply_);
	reply_->setInfoText("");*/
}

void OutputDirProvider::slotOutputClientError(QString msg)
{
	if(info_ && info_.get())
	{
		if(ServerHandler* server=info_->server())
		{
			if(outClient_ && !server->isLocalHost())
		    {
				//Check if it is tryno 0
				bool tynozero=(boost::algorithm::ends_with(outClient_->remoteFile(),".0"));
				VDir_ptr dir=fetchLocalDir(outClient_->remoteFile(),tynozero);
				if(dir)
				{
					reply_->setDirectory(dir);
					owner_->infoReady(reply_);
					return;
				}
		    }
		}

		reply_->setErrorText(msg.toStdString());
		owner_->infoFailed(reply_);
	}
}

VDir_ptr OutputDirProvider::fetchLocalDir(const std::string& path,bool trynozero)
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
		if((trynozero || boost::filesystem::exists(p)) &&
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

OutputDirClient* OutputDirProvider::makeOutputClient(const std::string& host,const std::string& port)
{
	if(outClient_)
	{
		if(outClient_->host() != host || outClient_->portStr() != port)
		{
			delete outClient_;
			outClient_=0;
		}
	}

	if(!outClient_)
	{
		outClient_=new OutputDirClient(host,port,this);

		connect(outClient_,SIGNAL(error(QString)),
				this,SLOT(slotOutputClientError(QString)));

		connect(outClient_,SIGNAL(progress(QString)),
				this,SLOT(slotOutputClientProgress(QString)));

		connect(outClient_,SIGNAL(finished()),
				this,SLOT(slotOutputClientFinished()));
	}

	return outClient_;


}




