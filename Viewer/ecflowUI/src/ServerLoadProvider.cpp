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
#include "ServerLoadProvider.hpp"

#include "VNode.hpp"
#include "VReply.hpp"
#include "ServerHandler.hpp"
#include "File.hpp"
#include "File.hpp"
#include "Str.hpp"

ServerLoadProvider::ServerLoadProvider(InfoPresenter* owner) :
    InfoProvider(owner,VTask::HistoryTask)
{

}

void ServerLoadProvider::clear()
{
    InfoProvider::clear();
}

void ServerLoadProvider::setAutoUpdate(bool autoUpdate)
{
    InfoProvider::setAutoUpdate(autoUpdate);

    if(active_)
    {
        if(!autoUpdate_)
        {
            //stopWatchFile();
        }
        else
        {
            if(!inAutoUpdate_)
                fetchFile();
        }
    }
    else
    {
        //stopWatchFile();
    }
}

void ServerLoadProvider::visit(VInfoServer* info)
{
    fetchFile();
}

void ServerLoadProvider::fetchFile()
{
    if(!active_)
        return;

    //stopWatchFile();

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

void ServerLoadProvider::fetchFile(ServerHandler *server,const std::string& fileName)
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
