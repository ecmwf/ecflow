//============================================================================
//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "ManualItemWidget.hpp"

#include "Highlighter.hpp"
#include "InfoProvider.hpp"
#include "MessageLabel.hpp"
#include "VConfig.hpp"
#include "VReply.hpp"

ManualItemWidget::ManualItemWidget(QWidget *parent) : CodeItemWidget(parent)
{
    fileLabel_->hide();
    messageLabel_->hide();
    textEdit_->setShowLineNumbers(false);

    Highlighter* ih=new Highlighter(textEdit_->document(),"manual");

    infoProvider_=new ManualProvider(this);

	//Editor font
	textEdit_->setFontProperty(VConfig::instance()->find("panel.manual.font"));
}

ManualItemWidget::~ManualItemWidget()
{
}

QWidget* ManualItemWidget::realWidget()
{
    return this;
}

void ManualItemWidget::reload(VInfo_ptr info)
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

void ManualItemWidget::clearContents()
{
    InfoPanelItem::clear();
    textEdit_->clear();
    messageLabel_->hide();

}

void ManualItemWidget::infoReady(VReply* reply)
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
    else if(s.isEmpty())
    {
    	messageLabel_->showInfo("Manual is <b>not</b> available");
    }

    fileLabel_->update(reply);
}

void ManualItemWidget::infoProgress(VReply* reply)
{
   // QString s=QString::fromStdString(reply->text());
    messageLabel_->showInfo(QString::fromStdString(reply->infoText()));
}

void ManualItemWidget::infoFailed(VReply* reply)
{
    QString s=QString::fromStdString(reply->errorText());
    messageLabel_->showError(s);
}

static InfoPanelItemMaker<ManualItemWidget> maker1("manual");

