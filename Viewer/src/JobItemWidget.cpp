//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "JobItemWidget.hpp"

#include "Highlighter.hpp"
#include "InfoProvider.hpp"
#include "VConfig.hpp"
#include "VReply.hpp"
#include "VNode.hpp"

JobItemWidget::JobItemWidget(QWidget *parent) : CodeItemWidget(parent)
{
	messageLabel_->hide();

	//Remove the first spacer item!!
	removeSpacer();

	Highlighter* ih=new Highlighter(textEdit_->document(),"job");

    infoProvider_=new JobProvider(this);

	//Editor font
	textEdit_->setFontProperty(VConfig::instance()->find("panel.job.font"));
}

JobItemWidget::~JobItemWidget()
{
}

QWidget* JobItemWidget::realWidget()
{
    return this;
}

void JobItemWidget::reload(VInfo_ptr info)
{
    assert(active_);

    if(suspended_)
        return;

    clearContents();
    info_=info;
    messageLabel_->hide();

    //Info must be a node
    if(info_ && info_->isNode() && info_->node())
    {
        infoProvider_->info(info_);
    }   
}

void JobItemWidget::clearContents()
{
    InfoPanelItem::clear();
    textEdit_->clear();
    messageLabel_->hide();
}

void JobItemWidget::infoReady(VReply* reply)
{
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

    fileLabel_->update(reply);
}

void JobItemWidget::infoProgress(VReply* reply)
{
    QString s=QString::fromStdString(reply->infoText());
    messageLabel_->showInfo(s);
}

void JobItemWidget::infoFailed(VReply* reply)
{
    QString s=QString::fromStdString(reply->errorText());
    messageLabel_->showError(s);
}

static InfoPanelItemMaker<JobItemWidget> maker1("job");
