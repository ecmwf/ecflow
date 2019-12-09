//============================================================================
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "JobStatusItemWidget.hpp"

#include <QTimer>

#include "Highlighter.hpp"
#include "InfoProvider.hpp"
#include "MessageLabel.hpp"
#include "ServerHandler.hpp"
#include "VConfig.hpp"
#include "VNode.hpp"
#include "VReply.hpp"
#include "CommandHandler.hpp"

JobStatusItemWidget::JobStatusItemWidget(QWidget *parent) :
    CodeItemWidget(parent),
    timeout_(3000),
    timeoutCount_(0),
    maxTimeoutCount_(10),
    taskMode_(NoTask),
    nodeStatusMode_(UnsetCommandMode)
{
    commandTb_->show();
    commandTb_->setText(tr("Execute status command"));

    statusCommandLabel_ = new MessageLabel(this);
    statusCommandLabel_->setShowTypeTitle(false);
    verticalLayout->insertWidget(1, statusCommandLabel_);
    statusCommandLabel_->hide();

    messageLabel_->setShowTypeTitle(false);
    messageLabel_->hide();
    textEdit_->setShowLineNumbers(false);

    infoProvider_=new JobStatusFileProvider(this);
    statusProvider_=new JobStatusProvider(this);

    //Editor font
    textEdit_->setFontProperty(VConfig::instance()->find("panel.job_status.font"));

    timer_ = new QTimer(this);
    timer_->setSingleShot(true);
    connect(timer_, SIGNAL(timeout()), this, SLOT(fetchJobStatusFile()));
}

JobStatusItemWidget::~JobStatusItemWidget()
= default;

QWidget* JobStatusItemWidget::realWidget()
{
    return this;
}

void JobStatusItemWidget::reload(VInfo_ptr info)
{
    assert(active_);

    if(suspended_)
        return;

    clearContents();

    statusCommandLabel_->hide();
    messageLabel_->hide();

    if(info && info->server() && info->server()->isDisabled())
    {
        setEnabled(false);
        return;
    }
    else
    {
        setEnabled(true);
    }

    //set the info
    adjust(info);

    //Info must be a node
    if(info_ && info_->isNode() && info_->node())
    {       
        startFileFetchTask();
    }
}

void JobStatusItemWidget::startFileFetchTask()
{
    Q_ASSERT(taskMode_==NoTask);
    taskMode_ = NoTask;
    if(info_ && info_->isNode() && info_->node())
    {
        VNode *node = info_->node();
        nodeStatusMode_ = (node->isActive() || node->isSubmitted())?EnabledCommandMode:DisabledCommandMode;

        // the status file could be partially generated so we need to fetch it
        if(node->isFlagSet(ecf::Flag::STATUSCMD_FAILED))
        {
            taskMode_=FetchFileTask;
            QString err = "Previous --status command has failed, check script/path/permissions!";
            statusCommandLabel_->showError(err);
            reloadTb_->setEnabled(false);
            commandTb_->setEnabled(false);
            infoProvider_->info(info_);
        }
        else if(nodeStatusMode_ == EnabledCommandMode)
        {
            if(!node->isFlagSet(ecf::Flag::STATUS))
            {
                QString err = "The --status command has not been run yet!";
                statusCommandLabel_->showWarning(err);
                reloadTb_->setEnabled(false);
                commandTb_->setEnabled(nodeStatusMode_ == EnabledCommandMode);
            }
            else
            {
                taskMode_=FetchFileTask;
                reloadTb_->setEnabled(false);
                commandTb_->setEnabled(false);
                infoProvider_->info(info_);
            }
        }
        else
        {
            QString warn = "The --status command can only be run for <b>active</b> or <b>submitted</b> nodes!";
            statusCommandLabel_->showWarning(warn);
            reloadTb_->setEnabled(false);
            commandTb_->setEnabled(false);
        }
     }
}

void JobStatusItemWidget::finishFileFetchTask()
{
    Q_ASSERT(taskMode_==NoTask);
    if(info_ && info_->isNode() && info_->node())
    {
        VNode *node = info_->node();
        if(node->isFlagSet(ecf::Flag::STATUSCMD_FAILED))
        {
            QString err = "Previous --status command has failed, check script/path/permissions!";
            statusCommandLabel_->showError(err);
            reloadTb_->setEnabled(nodeStatusMode_ == EnabledCommandMode);
            commandTb_->setEnabled(nodeStatusMode_ == EnabledCommandMode);
        }
        else if(!node->isFlagSet(ecf::Flag::STATUS))
        {
            QString err = "The --status command has not been run yet!";
            statusCommandLabel_->showWarning(err);
            reloadTb_->setEnabled(false);
            commandTb_->setEnabled(nodeStatusMode_ == EnabledCommandMode);
        }
        else
        {
            reloadTb_->setEnabled(true);
            commandTb_->setEnabled(nodeStatusMode_ == EnabledCommandMode);
        }
    }
}


void JobStatusItemWidget::startStatusCommandTask()
{
    if(taskMode_==NoTask && nodeStatusMode_ == EnabledCommandMode &&
       info_ && info_->isNode() && info_->node())
    {
        Q_ASSERT(nodeStatusMode_ == EnabledCommandMode);
        Q_ASSERT(timeoutCount_ == 0);
        Q_ASSERT(!timer_->isActive());
        taskMode_=StatusCommandTask;
        commandTb_->setEnabled(false);
        reloadTb_->setEnabled(false);
        statusCommandLabel_->showInfo("Generating job status information ...");
        statusCommandLabel_->startLoadLabel();

        timer_->stop();
        timeoutCount_ = 0;
        statusProvider_->info(info_);
    }
}

//returns true if the task has finished/failed
bool JobStatusItemWidget::checkStatusCommandTask(VReply* reply)
{
    Q_ASSERT(reply);

    QString s=QString::fromStdString(reply->text());
    if (taskMode_ == StatusCommandTask)
    {
        Q_ASSERT(nodeStatusMode_ == EnabledCommandMode);
        VNode *node = nullptr;
        if (info_ && info_->isNode() && info_->node())
        {
            node = info_->node();
            // check if the node status changed significantly
            bool st=(node->isActive() || node->isSubmitted());
            if ((st && nodeStatusMode_ != EnabledCommandMode) ||
                 (!st && nodeStatusMode_ != DisabledCommandMode))
            {
                reload(info_);
            }
        }
        else {
            reload(info_);
            return false;
        }

        //the status command has just finished
        if(reply->sender() == statusProvider_)
        {
            Q_ASSERT(timeoutCount_ ==0);
            Q_ASSERT(!timer_->isActive());
            if( reply->status() == VReply::TaskFailed)
            {
                taskMode_=NoTask;
                QString err = QString::fromStdString(reply->errorText());
                statusCommandLabel_->stopLoadLabel();
                statusCommandLabel_->showError(err);
                reloadTb_->setEnabled(true);
                commandTb_->setEnabled(true);
                timeoutCount_ = 0;
                timer_->stop();
            }
            else{
                timeoutCount_ = 0;
                timer_->start(timeout_);
            }
            return false;
        }

        //here we must be fetching the status file
        Q_ASSERT(node);
        Q_ASSERT(reply->sender() == infoProvider_);
        if(node->isFlagSet(ecf::Flag::STATUSCMD_FAILED))
        {
            taskMode_=NoTask;
            QString err = "Previous --status command has failed, check path/permissions!";
            statusCommandLabel_->showError(err);
            statusCommandLabel_->stopLoadLabel();
            reloadTb_->setEnabled(true);
            commandTb_->setEnabled(true);
            timeoutCount_ = 0;
            timer_->stop();
            return true;
        }
        else if(s.isEmpty())
        {           
            timeoutCount_++;
            if (timeoutCount_ < maxTimeoutCount_)
            {
                timer_->start(timeout_);
                return false;
            }
            else
            {
                taskMode_=NoTask;
                reloadTb_->setEnabled(true);
                commandTb_->setEnabled(true);
                statusCommandLabel_->stopLoadLabel();
                statusCommandLabel_->hide();
                timeoutCount_ = 0;
                timer_->stop();
                return true;
            }          
        }
        else
        {
            taskMode_=NoTask;
            reloadTb_->setEnabled(true);
            commandTb_->setEnabled(true);
            statusCommandLabel_->stopLoadLabel();
            statusCommandLabel_->hide();
            timeoutCount_ = 0;
            timer_->stop();
            return true;
        }
    }
    return true;
}

void JobStatusItemWidget::fetchJobStatusFile()
{
    Q_ASSERT(taskMode_==StatusCommandTask);
    if(info_ && info_->isNode() && info_->node())
    {
        reloadTb_->setEnabled(false);
        infoProvider_->info(info_);
    }
}

void JobStatusItemWidget::clearContents()
{
    taskMode_=NoTask;
    nodeStatusMode_=UnsetCommandMode;
    statusCommandLabel_->stopLoadLabel();
    timer_->stop();
    timeoutCount_ = 0;
    InfoPanelItem::clear();
    textEdit_->clear();
    messageLabel_->hide();
    reloadTb_->setEnabled(false);
    commandTb_->setEnabled(false);
    clearCurrentFileName();
}

void JobStatusItemWidget::infoReady(VReply* reply)
{
    Q_ASSERT(reply);

    if(!checkStatusCommandTask(reply))
    {
        return;
    }

    if(taskMode_ == FetchFileTask) {      
        taskMode_ = NoTask;
        finishFileFetchTask();
    }

    Q_ASSERT(taskMode_ == NoTask);

    QString s=QString::fromStdString(reply->text());
    textEdit_->setPlainText(s);

    if(reply->hasWarning())
    {
        messageLabel_->showWarning(QString::fromStdString(reply->warningText()));
    }
    else if(reply->hasInfo())
    {
        messageLabel_->showInfo(QString::fromStdString(reply->infoText()));
    }
    else if(s.isEmpty())
    {
        messageLabel_->showInfo("Job status is <b>not</b> available");
    }
    else
    {
        messageLabel_->hide();
    }

    fileLabel_->update(reply);
    setCurrentFileName(reply->fileName());
}

void JobStatusItemWidget::infoProgress(VReply* reply)
{
}

void JobStatusItemWidget::infoFailed(VReply* reply)
{    
    if(checkStatusCommandTask(reply))
    {    
        if(taskMode_ == FetchFileTask) {
            taskMode_ = NoTask;
            finishFileFetchTask();
        }

        Q_ASSERT(taskMode_ == NoTask);
        QString s=QString::fromStdString(reply->errorText());
        messageLabel_->showError(s);
    }
}

void JobStatusItemWidget::reloadRequested()
{
    startFileFetchTask();
}

void JobStatusItemWidget::commandRequested()
{
    startStatusCommandTask();
}

void JobStatusItemWidget::updateState(const FlagSet<ChangeFlag>& flags)
{
    if(flags.isSet(SuspendedChanged))
    {
        //Suspend
        if(suspended_)
        {
            reloadTb_->setEnabled(false);
            commandTb_->setEnabled(false);
            if (taskMode_ ==  StatusCommandTask)
            {
                timer_->stop();
                messageLabel_->stopLoadLabel();
                messageLabel_->hide();
            }
        }
        //Resume
        else
        {
            if(info_ && info_->node())
            {
                reload(info_);
            }
            else
            {
                clearContents();
            }
        }
    }
}

void JobStatusItemWidget::nodeChanged(const VNode* n, const std::vector<ecf::Aspect::Type>& aspect)
{
    //Changes in the nodes
    for(auto it : aspect)
    {
        if(it == ecf::Aspect::STATE || it == ecf::Aspect::DEFSTATUS ||
            it == ecf::Aspect::SUSPENDED)
        {
            if(info_ && info_->isNode() && info_->node())
            {
                VNode *node = info_->node();
                bool st=(node->isActive() || node->isSubmitted());
                if ((st && nodeStatusMode_ != EnabledCommandMode) ||
                     (!st && nodeStatusMode_ != DisabledCommandMode))
                    {
                        if(taskMode_ == NoTask)
                        {
                            reload(info_);
                        }
                    }
            }
        }
    }
}

static InfoPanelItemMaker<JobStatusItemWidget> maker1("job_status");
