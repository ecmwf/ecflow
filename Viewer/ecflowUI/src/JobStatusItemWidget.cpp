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
    fetchFileScheduled_(false)
{
    commandTb_->show();
    commandTb_->setText(tr("Execute --status command"));

    messageLabel_->setShowTypeTitle(false);
    messageLabel_->hide();
    textEdit_->setShowLineNumbers(false);

    infoProvider_=new JobStatusProvider(this);

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
        fetchJobStatusFile();
    }
}

void JobStatusItemWidget::fetchJobStatusFile()
{
    if(info_ && info_->isNode() && info_->node())
    {
        reloadTb_->setEnabled(false);
        infoProvider_->info(info_);
    }
}

void JobStatusItemWidget::clearContents()
{
    fetchFileScheduled_ = false;
    messageLabel_->stopLoadLabel();
    timer_->stop();
    timeoutCount_ = 0;
    InfoPanelItem::clear();
    textEdit_->clear();
    messageLabel_->hide();
    reloadTb_->setEnabled(true);
    clearCurrentFileName();
}

void JobStatusItemWidget::infoReady(VReply* reply)
{
    Q_ASSERT(reply);
    QString s=QString::fromStdString(reply->text());
    textEdit_->setPlainText(s);

    // handle status command
    if(s.isEmpty() && prolongStatusCommand())
        return;
    else
        stopStatusCommand();

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

    fileLabel_->update(reply);
    reloadTb_->setEnabled(true);
    setCurrentFileName(reply->fileName());
}

void JobStatusItemWidget::infoProgress(VReply* reply)
{
    messageLabel_->showInfo(QString::fromStdString(reply->infoText()));
}

void JobStatusItemWidget::infoFailed(VReply* reply)
{
    if(!prolongStatusCommand())
    {
        QString s=QString::fromStdString(reply->errorText());
        messageLabel_->showError(s);
        reloadTb_->setEnabled(true);
    }
}

void JobStatusItemWidget::reloadRequested()
{
    fetchJobStatusFile();
}

void JobStatusItemWidget::commandRequested()
{
    runStatusCommand();
}

void JobStatusItemWidget::stopStatusCommand()
{
    if(timeoutCount_ > 0)
    {
        commandTb_->setEnabled(true);
        reloadTb_->setEnabled(true);
        messageLabel_->stopLoadLabel();
        messageLabel_->hide();
        timeoutCount_ = 0;
        timer_->stop();
    }
}

bool JobStatusItemWidget::prolongStatusCommand()
{
    if (timeoutCount_ > 0)
    {
        timeoutCount_++;
        if (timeoutCount_ < maxTimeoutCount_)
        {
            timer_->start(timeout_);
            return true;
        }
        else
        {
            stopStatusCommand();
            return false;
        }
    }
    return false;
}

void JobStatusItemWidget::updateState(const FlagSet<ChangeFlag>& flags)
{
    if(flags.isSet(SuspendedChanged))
    {
        //Suspend
        if(suspended_)
        {
            reloadTb_->setEnabled(false);
            if (timer_->isActive()) {
                timer_->stop();
                messageLabel_->stopLoadLabel();
                messageLabel_->hide();
                fetchFileScheduled_ = true;
            }
        }
        //Resume
        else
        {
            if(info_ && info_->node())
            {
                reloadTb_->setEnabled(true);
                if (fetchFileScheduled_) {
                    fetchFileScheduled_ = false;
                    reload(info_);
                }
            }
            else
            {
                clearContents();
            }
        }
    }
}

void JobStatusItemWidget::runStatusCommand()
{
    if(info_ && info_->isNode() && info_->node() && timeoutCount_ == 0)
    {
        commandTb_->setEnabled(false);
        reloadTb_->setEnabled(false);
        messageLabel_->showInfo("Generating job status information ...");
        messageLabel_->startLoadLabel();

        std::vector<std::string> cmd;
        cmd.emplace_back("ecflow_client");
        cmd.emplace_back("--status");
        cmd.emplace_back("<full_name>");
        CommandHandler::run(info_,cmd);

        timeoutCount_ = 1;
        timer_->start(timeout_);
    }
}

static InfoPanelItemMaker<JobStatusItemWidget> maker1("job_status");
