//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include <fstream>

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include "LogProvider.hpp"

#include "FileWatcher.hpp"
#include "VNode.hpp"
#include "VReply.hpp"
#include "ServerHandler.hpp"
#include "File.hpp"


LogProvider::LogProvider(InfoPresenter* owner,QObject* parent) :
	QObject(parent),
	InfoProvider(owner,VTask::HistoryTask),
    fileWatcher_(0)
{
    //NOTE: fileWatcher_'s parent (if it exists) will be "this", so
	//fileWatcher_ will be automatically deleted in the destructor!
}

void LogProvider::clear()
{
	stopWatchFile();
	InfoProvider::clear();
}

void LogProvider::setAutoUpdate(bool autoUpdate)
{
    InfoProvider::setAutoUpdate(autoUpdate);

    if(active_)
    {
        if(!autoUpdate_)
        {
            stopWatchFile();
        }
        else
        {
            if(!inAutoUpdate_)
                fetchFile();
        }
    }
    else
    {
        stopWatchFile();
    }
}

void LogProvider::visit(VInfoServer* info)
{
	fetchFile();
}

void LogProvider::fetchFile()
{
    if(!active_)
		return;

	stopWatchFile();

	//Reset the reply
	reply_->reset();

    if(!info_)
	{
	   owner_->infoFailed(reply_);
	   return;
	}

	ServerHandler* server=info_->server();

	//Get the filename
	std::string fileName=server->vRoot()->genVariable("ECF_LOG");

	fetchFile(server,fileName);
}

void LogProvider::fetchFile(ServerHandler *server,const std::string& fileName)
{
	if(!server)
    {
    	owner_->infoFailed(reply_);
    	return;
    }

	//Set the filename in reply
	reply_->fileName(fileName);

	//No filename is available
	if(fileName.empty())
	{
		reply_->setErrorText("Variable ECF_LOG is not defined!");
		owner_->infoFailed(reply_);
	}

    //First we try to read the file directly from the disk
    //if(server->readFromDisk())
    {
    	size_t file_size = 0;
    	std::string err_msg;
    	reply_->text( ecf::File::get_last_n_lines(fileName,100,file_size,err_msg));
    	if(err_msg.empty())
    	{
    		reply_->fileReadMode(VReply::LocalReadMode);

    		if(autoUpdate_)
    			inAutoUpdate_=true;

    		owner_->infoReady(reply_);

    		//Try to track the changes in the log file
    		watchFile(fileName,file_size);
    		return;
    	}
    }

    //Finally we try the server
    reply_->fileReadMode(VReply::ServerReadMode);

    //Define a task for getting the info from the server.
    task_=VTask::create(taskType_,server->vRoot(),this);

    //Run the task in the server. When it finish taskFinished() is called. The text returned
    //in the reply will be prepended to the string we generated above.
    server->run(task_);

#if 0
    //If we are we could not get the file
    //owner_->infoFailed(reply_);
#endif
}

void LogProvider::watchFile(const std::string& fileName,size_t offset)
{
	if(autoUpdate_)
	{
		assert(fileWatcher_ == 0);
		fileWatcher_=new FileWatcher(fileName,offset,this);

		connect(fileWatcher_,SIGNAL(linesAppended(QStringList)),
			this,SLOT(slotLinesAppend(QStringList)));

		inAutoUpdate_=true;
	}
}

void LogProvider::stopWatchFile()
{
	if(fileWatcher_)
	{
		delete fileWatcher_;
		fileWatcher_=0;
	}

	inAutoUpdate_=false;
}

void LogProvider::slotLinesAppend(QStringList lst)
{
	//Check if the task is already running
	if(task_)
	{
		task_->status(VTask::CANCELLED);
		task_.reset();
	}

	//Reset the reply
	reply_->reset();

	if(!info_)
	{
		owner_->infoFailed(reply_);
	}

	std::vector<std::string> vec;
	Q_FOREACH(QString s,lst)
	{
		vec.push_back(s.toStdString());
	}

	reply_->setTextVec(vec);
	owner_->infoAppended(reply_);
}

