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
#include "VReply.hpp"
#include "CommandHandler.hpp"

JobStatusItemWidget::JobStatusItemWidget(QWidget *parent) :
    CodeItemWidget(parent),
    timeout_(2000)
{
    messageLabel_->setShowTypeTitle(false);
    messageLabel_->hide();
    textEdit_->setShowLineNumbers(false);

    //The document becomes the owner of the highlighter
    new Highlighter(textEdit_->document(), "manual");

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
        reloadTb_->setEnabled(false);
        runStatusCommand();
        timer_->start(timeout_);
    }
}

void JobStatusItemWidget::fetchJobStatusFile()
{
    timer_->stop();
    if(info_ && info_->isNode() && info_->node())
    {
        reloadTb_->setEnabled(false);
        infoProvider_->info(info_);
    }
}

void JobStatusItemWidget::clearContents()
{
    timer_->stop();
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
    QString s=QString::fromStdString(reply->errorText());
    messageLabel_->showError(s);
    reloadTb_->setEnabled(true);
}

void JobStatusItemWidget::reloadRequested()
{
    reload(info_);
}

void JobStatusItemWidget::updateState(const FlagSet<ChangeFlag>& flags)
{
    if(flags.isSet(SuspendedChanged))
    {
        //Suspend
        if(suspended_)
        {
            reloadTb_->setEnabled(false);
        }
        //Resume
        else
        {
            if(info_ && info_->node())
            {
                reloadTb_->setEnabled(true);
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
    std::vector<std::string> cmd;
    cmd.emplace_back("ecflow_client");
    cmd.emplace_back("--status");
    cmd.emplace_back("<full_name>");
    CommandHandler::run(info_,cmd);
}

static InfoPanelItemMaker<JobStatusItemWidget> maker1("job_status");