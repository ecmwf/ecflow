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
    taskMode_(NoTask)
{
    commandTb_->show();
    commandTb_->setText(tr("Execute status command"));

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

    info_=info;

    //Info must be a node
    if(info_ && info_->isNode() && info_->node())
    {       
        startFileFetchTask();
    }
}

void JobStatusItemWidget::startFileFetchTask()
{
    if(info_ && info_->isNode() && info_->node())
    {
        VNode *node = info_->node();
        if(node->isFlagSet(ecf::Flag::STATUSCMD_FAILED))
        {
            taskMode_=NoTask;
            QString err = "Previous --status command has failed, check path/permissions!";
            messageLabel_->showError(err);
            reloadTb_->setEnabled(true);
            commandTb_->setEnabled(node->isActive() || node->isSubmitted());
            return;
        }
        taskMode_=FetchFileTask;
        commandTb_->setEnabled(false);
        reloadTb_->setEnabled(false);
        infoProvider_->info(info_);
    }
}

void JobStatusItemWidget::startStatusCommandTask()
{
    if(taskMode_==NoTask && info_ && info_->isNode() && info_->node())
    {
        Q_ASSERT(timeoutCount_ == 0);
        Q_ASSERT(!timer_->isActive());
        taskMode_=StatusCommandTask;
        commandTb_->setEnabled(false);
        reloadTb_->setEnabled(false);
        messageLabel_->showInfo("Generating job status information ...");
        messageLabel_->startLoadLabel();

        timer_->stop();
        timeoutCount_ = 0;
        statusProvider_->info(info_);

        /*std::vector<std::string> cmd;
        cmd.emplace_back("ecflow_client");
        cmd.emplace_back("--status");
        cmd.emplace_back("<full_name>");
        CommandHandler::run(info_,cmd);*/

        //timeoutCount_ = 1;
        //timer_->start(timeout_);
    }
}

//returns true if the task has finished/failed
bool JobStatusItemWidget::checkStatusCommandTask(VReply* reply)
{
    Q_ASSERT(reply);

    QString s=QString::fromStdString(reply->text());
    if (taskMode_ == StatusCommandTask)
    {
        VNode *node = nullptr;
        if (info_ && info_->isNode() && info_->node())
        {
            node = info_->node();
            Q_ASSERT(node);
        }

        //the status has just command finished
        if(reply->sender() == statusProvider_)
        {
            Q_ASSERT(timeoutCount_ ==0);
            Q_ASSERT(!timer_->isActive());
            if( reply->status() == VReply::TaskFailed)
            {
                taskMode_=NoTask;
                QString err = QString::fromStdString(reply->errorText());
                messageLabel_->stopLoadLabel();
                messageLabel_->showError(err);
                reloadTb_->setEnabled(true);
                commandTb_->setEnabled(node->isActive() || node->isSubmitted());
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
        Q_ASSERT(reply->sender() == infoProvider_);
        if(isStatusCmdFailedFlagSet())
        {
            taskMode_=NoTask;
            QString err = "Previous --status command has failed, check path/permissions!";
            messageLabel_->showError(err);
            reloadTb_->setEnabled(true);
            commandTb_->setEnabled(node->isActive() || node->isSubmitted());
            messageLabel_->stopLoadLabel();
            timeoutCount_ = 0;
            timer_->stop();
            return false;
        }
        else if(s.isEmpty())
        {
            //Q_ASSERT(timeoutCount_ > 0);
            //if (timeoutCount_ > 0)
            //{
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
                commandTb_->setEnabled(node->isActive() || node->isSubmitted());
                messageLabel_->stopLoadLabel();
                messageLabel_->hide();
                timeoutCount_ = 0;
                timer_->stop();
                return true;
            }
           // }
        }
        else
        {
            taskMode_=NoTask;
            reloadTb_->setEnabled(true);
            commandTb_->setEnabled(node->isActive() || node->isSubmitted());
            messageLabel_->stopLoadLabel();
            messageLabel_->hide();
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
    messageLabel_->stopLoadLabel();
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
    }

    Q_ASSERT(taskMode_ == NoTask);
    if(info_ && info_->isNode() && info_->node())
    {
        VNode* node = info_->node();
        Q_ASSERT(node);
        commandTb_->setEnabled(node->isActive() || node->isSubmitted());
    }

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

    if(taskMode_ == FetchFileTask) {
        taskMode_ = NoTask;
    }
    Q_ASSERT(taskMode_ == NoTask);

    reloadTb_->setEnabled(true);
    if(info_ && info_->isNode() && info_->node())
    {
        VNode* node = info_->node();
        Q_ASSERT(node);
        commandTb_->setEnabled(node->isActive() || node->isSubmitted());
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
        QString s=QString::fromStdString(reply->errorText());
        messageLabel_->showError(s);

        if(taskMode_ == FetchFileTask) {
            taskMode_ = NoTask;
        }
        Q_ASSERT(taskMode_ == NoTask);

        reloadTb_->setEnabled(true);
        if(info_ && info_->isNode() && info_->node())
        {
            VNode* node = info_->node();
            Q_ASSERT(node);
            commandTb_->setEnabled(node->isActive() || node->isSubmitted());
        }
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
                if (taskMode_ == StatusCommandTask)
                {
                    commandTb_->setEnabled(false);
                    reloadTb_->setEnabled(false);
                    messageLabel_->showInfo("Generating job status information ...");
                    messageLabel_->startLoadLabel();
                    if (timeoutCount_ > 0)
                    {
                        timer_->start(timeout_);
                    }
                }
                else if (taskMode_ == NoTask)
                {
                    reloadTb_->setEnabled(true);
                    if(info_ && info_->isNode() && info_->node())
                    {
                        VNode* node = info_->node();
                        Q_ASSERT(node);
                        commandTb_->setEnabled(node->isActive() || node->isSubmitted());
                    }
                }
                else
                {
                    reloadTb_->setEnabled(false);
                    commandTb_->setEnabled(false);
                }
            }
            else
            {
                clearContents();
            }
        }
    }
}

bool JobStatusItemWidget::isStatusCmdFailedFlagSet() const
{
    if(info_ && info_->isNode() && info_->node())
    {
        VNode *node = info_->node();
        return node->isFlagSet(ecf::Flag::STATUSCMD_FAILED);
    }
    return false;
}


static InfoPanelItemMaker<JobStatusItemWidget> maker1("job_status");
