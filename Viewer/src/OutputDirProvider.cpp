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

#include "LogServer.hpp"
#include "OutputDirClient.hpp"
#include "VNode.hpp"
#include "VReply.hpp"
#include "ServerHandler.hpp"
#include "UiLog.hpp"

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
        return;
   	}

	ServerHandler* server=info_->server();
	VNode *n=info->node();

    if(!n || !n->node())
   	{
       	owner_->infoFailed(reply_);
        return;
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
        return;
	}

    if(!n)
    {
        reply_->setDirectory(dir);
        owner_->infoFailed(reply_);
        return;
    }

	//Get the jobout name
	std::string fileName=n->findVariable("ECF_JOBOUT",true);

	//Check if it is tryno 0
    //bool tynozero=(boost::algorithm::ends_with(fileName,".0"));

	//Jobout is empty: no dir path is availabale
	if(fileName.empty())
	{
		reply_->setDirectory(dir);
		owner_->infoFailed(reply_);
		return;
	}

    //We try the output client, its asynchronous!
    if(fetchDirViaOutputClient(n,fileName))
    {
      	//If we are here we created a output client and asked to the fetch the
      	//file asynchronously. The ouput client will call slotOutputClientFinished() or
      	//slotOutputClientError eventually!!
      	return;
    }

    //If there is no output client we try to read it from disk
    dir=fetchLocalDir(fileName);
    if(dir)
    {
        reply_->setDirectory(dir);
        owner_->infoReady(reply_);
        return;
    }

    //If we are here the error or warning is already set in reply
    reply_->setDirectory(dir);
    owner_->infoFailed(reply_);
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
	VDir_ptr dir = outClient_->result();

	if(dir && dir.get())
	{
        dir->setFetchMode(VDir::LogServerFetchMode);
        std::string method="served by " + outClient_->host() + "@" + outClient_->portStr();
        dir->setFetchModeStr(method);
        dir->setFetchDate(QDateTime::currentDateTime());

        reply_->setInfoText("");
		reply_->setDirectory(dir);
		owner_->infoReady(reply_);
	}
}

void OutputDirProvider::slotOutputClientProgress(QString,int)
{
}

void OutputDirProvider::slotOutputClientError(QString msg)
{
    if(info_)
	{
        std::string sDesc;
        if(outClient_)
        {
            sDesc="Failed to fetch from " + outClient_->host() + "@" + outClient_->portStr();
            if(!msg.isEmpty())
                sDesc+=" error: " + msg.toStdString();
            reply_->setErrorText(sDesc);

            VDir_ptr dir=fetchLocalDir(outClient_->remoteFile());
            if(dir)
            {
                dir->setFetchDate(QDateTime::currentDateTime());
                dir->setFetchMode(VDir::LocalFetchMode);
                reply_->setErrorText("");
                reply_->setDirectory(dir);
                owner_->infoReady(reply_);
                return;
            }
		}
        else
        {
            sDesc="Failed to fetch from logserver";
            if(!msg.isEmpty())
                sDesc+=": " + msg.toStdString();
            reply_->setErrorText(sDesc);
        }

		owner_->infoFailed(reply_);
	}
}

VDir_ptr OutputDirProvider::fetchLocalDir(const std::string& path)
{
	VDir_ptr res;

	boost::filesystem::path p(path);

	try {
		//Is it a directory?
		if(boost::filesystem::is_directory(p))
		{			
            return res;
		}

        if(boost::filesystem::exists(p.parent_path()))
		{
			std::string dirName=p.parent_path().string();           
            if(info_ && info_->isNode() && info_->node())
            {
                std::string nodeName=info_->node()->strName();
                std::string pattern=nodeName+".";
				res=VDir_ptr(new VDir(dirName,pattern));
                res->setFetchDate(QDateTime::currentDateTime());
                res->setFetchMode(VDir::LocalFetchMode);
                return res;
            }
        }

        std::string msg("No access to path on disk!");
        reply_->appendErrorText(msg);
	}
	catch (const boost::filesystem::filesystem_error& e)
	{
        std::string msg;
        msg+="No access to path on disk! error: " + std::string(e.what());
        reply_->appendErrorText(msg);
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
			outClient_=0;
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
