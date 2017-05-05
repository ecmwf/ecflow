//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "ScriptItemWidget.hpp"

#include "Highlighter.hpp"
#include "InfoProvider.hpp"
#include "MessageLabel.hpp"
#include "VConfig.hpp"
#include "VNode.hpp"
#include "VReply.hpp"

//========================================================
//
// ScriptItemWidget
//
//========================================================

ScriptItemWidget::ScriptItemWidget(QWidget *parent) : CodeItemWidget(parent)
{
    messageLabel_->hide();
    
    //Remove the first spacer item!!
    removeSpacer();
    
    Highlighter* ih=new Highlighter(textEdit_->document(),"script");

	infoProvider_=new ScriptProvider(this);

	//Editor font
	textEdit_->setFontProperty(VConfig::instance()->find("panel.script.font"));
}

ScriptItemWidget::~ScriptItemWidget()
{
}

QWidget* ScriptItemWidget::realWidget()
{
	return this;
}

void ScriptItemWidget::reload(VInfo_ptr info)
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
        reloadTb_->setEnabled(false);
        infoProvider_->info(info_);
    }
}

void ScriptItemWidget::clearContents()
{
    InfoPanelItem::clear();
    fileLabel_->clear();
    textEdit_->clear();
    messageLabel_->hide();
    reloadTb_->setEnabled(true);
}

void ScriptItemWidget::infoReady(VReply* reply)
{
    messageLabel_->hide();
    
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
    reloadTb_->setEnabled(true);
}

void ScriptItemWidget::infoProgress(VReply* reply)
{
    QString s=QString::fromStdString(reply->text());
    messageLabel_->showInfo(QString::fromStdString(reply->infoText()));
}

void ScriptItemWidget::infoFailed(VReply* reply)
{
    QString s=QString::fromStdString(reply->errorText());
    //textEdit_->setPlainText(s);   
    messageLabel_->showError(s);
    reloadTb_->setEnabled(true);
}

void ScriptItemWidget::reloadRequested()
{
    reload(info_);
}

void ScriptItemWidget::updateState(const FlagSet<ChangeFlag>& flags)
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

static InfoPanelItemMaker<ScriptItemWidget> maker1("script");
