//============================================================================
// Copyright 2009-2018 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "VReportMaker.hpp"

#include "ServerHandler.hpp"
#include "ShellCommand.hpp"
#include "VNode.hpp"
#include "VReply.hpp"
#include "OutputFileProvider.hpp"
#include "UiLog.hpp"

VReportMaker::VReportMaker(QObject *parent) : QObject(parent)
{
    infoProvider_=new OutputFileProvider(this);
}

void VReportMaker::run(VInfo_ptr info)
{
    VNode *node=info->node();

    info_=info;

    //Get file contents
    infoProvider_->info(info);

    //Get job output
}

void VReportMaker::infoReady(VReply* reply)
{
    OutputFileProvider* op=static_cast<OutputFileProvider*>(infoProvider_);

    if(reply->fileName() == op->joboutFileName() && !op->isTryNoZero(reply->fileName()) &&
       info_ && info_->isNode() && info_->node() && info_->node()->isSubmitted())
    {
#if 0
        hasMessage=true;
        submittedWarning_=true;
        messageLabel_->showWarning("This is the current job output (as defined by variable ECF_JOBOUT), but \
               because the node status is <b>submitted</b> it may contain the ouput from a previous run!");
#endif
    }
    else
    {
#if 0
        if(reply->hasWarning())
        {
            messageLabel_->showWarning(QString::fromStdString(reply->warningText()));
            hasMessage=true;
        }
        else if(reply->hasInfo())
        {
            messageLabel_->showInfo(QString::fromStdString(reply->infoText()));
            hasMessage=true;
        }
#endif
    }

    VFile_ptr f=reply->tmpFile();
    sendReport(f);
    deleteLater();
}

void VReportMaker::infoFailed(VReply*)
{
    VFile_ptr f;
    sendReport(f);
    deleteLater();
}

void VReportMaker::sendReport(VFile_ptr file)
{
    if(info_->isNode() && info_->node())
    {
        VNode *node=info_->node();
        ServerHandler* s=info_->server();

        if(node && s)
        {
            std::string filePath="_undef_";
            if(file)
            {
                if(file->storageMode() != VFile::DiskStorage)
                    file->setStorageMode(VFile::DiskStorage);

                filePath=file->path();
            }

            UiLog().dbg() << "REPORT outfile=" << filePath;

            std::string cmd="sh ecflow_ui_create_jira_issue.sh \'" + filePath + "\' " +
                    s->host() + " " + s->port() + " \'" +
                    node->absNodePath() + "\' \'" + node->stateName().toStdString() + "\'";

            ShellCommand::run(cmd,"");
        }
    }
}

void VReportMaker::sendReport(VInfo_ptr info)
{
    VReportMaker* maker=new VReportMaker();
    maker->run(info);
    //maker will delete itself when the job is done!
}
