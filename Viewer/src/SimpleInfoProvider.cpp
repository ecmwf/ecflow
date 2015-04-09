//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "SimpleInfoProvider.hpp"

#include "ServerHandler.hpp"

SimpleInfoProvider::SimpleInfoProvider(InfoPresenter* owner,VTask::Type taskType) :
     InfoProvider(owner),
     taskType_(taskType)
{

}

//Node
void SimpleInfoProvider::visit(VInfoNode* info)
{
    reply_->reset();

    if(!info->node())
    {
        owner_->infoFailed(reply_);
    }

    //Define a task for getting the info from the server.
    task_=VTask::create(taskType_,info->node(),this);

    //Run the task in the server. When it finish taskFinished() is called. The text returned
    //in the reply will be prepended to the string we generated above.
    info->server()->run(task_);
}

void  SimpleInfoProvider::taskChanged(VTask_ptr task)
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
}






