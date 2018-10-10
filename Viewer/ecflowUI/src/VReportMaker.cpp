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
    if(f)
    {
        loadFile(f);

        //browser_->loadFile(f);
        //if(f->storageMode() == VFile::DiskStorage)
        //    hasMessage=false;

    }

    deleteLater();
}

void VReportMaker::infoFailed(VReply*)
{
    deleteLater();
}

void VReportMaker::loadFile(VFile_ptr file)
{
    if(!file)
    {
        //clear();
        return;
    }

    if(file->storageMode() == VFile::DiskStorage)
    {
        //loadFile(QString::fromStdString(file_->path()));
        UiLog().dbg() << "REPORT " << file->path();
    }
    else
    {
        QString s(file->data());
        //loadText(s,QString::fromStdString(file_->sourcePath()),true);
        UiLog().dbg() << "REPORT " << s;
    }
}



void VReportMaker::sendReport(VInfo_ptr info)
{
    VReportMaker* maker=new VReportMaker();
    maker->run(info);
    //maker will delete itself when the job is done!
}
