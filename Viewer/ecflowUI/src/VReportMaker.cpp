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

    //Get job output
    infoProvider_->info(info);
}

void VReportMaker::infoReady(VReply* reply)
{
    Q_ASSERT(reply);
    VFile_ptr f=reply->tmpFile();
    sendJiraReport(f);
    deleteLater();
}

void VReportMaker::infoFailed(VReply*)
{
    VFile_ptr f;
    sendJiraReport(f);
    deleteLater();
}

void VReportMaker::sendJiraReport(VFile_ptr file)
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
