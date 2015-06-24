//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "ScriptItemWidget.hpp"

#include "Highlighter.hpp"
#include "InfoProvider.hpp"
#include "VNode.hpp"
#include "VReply.hpp"

//========================================================
//
// ScriptItemWidget
//
//========================================================

ScriptItemWidget::ScriptItemWidget(QWidget *parent) : CodeItemWidget(parent)
{
    QFont f;
	f.setFamily("Monospace");
	//f.setFamily("Courier");
	f.setStyleHint(QFont::TypeWriter);
	f.setFixedPitch(true);
	textEdit_->setFont(f);

    Highlighter* ih=new Highlighter(textEdit_->document(),"script");

	infoProvider_=new ScriptProvider(this);
  
}

QWidget* ScriptItemWidget::realWidget()
{
	return this;
}

void ScriptItemWidget::reload(VInfo_ptr info)
{
    loaded_=true;
    info_=info;

    if(!info.get() || !info->isNode() || !info->node())
    {
        fileLabel_->clear();
        textEdit_->clear();
    }
    else
    {
        clearContents();
        fileLabel_->setText(tr("File: ") + QString::fromStdString(info->node()->genVariable("ECF_SCRIPT")));
        infoProvider_->info(info_);
    }	
}

void ScriptItemWidget::clearContents()
{
    loaded_=false;
    fileLabel_->clear();
    textEdit_->clear();
}

void ScriptItemWidget::infoReady(VReply* reply)
{
    QString s=QString::fromStdString(reply->text());
    textEdit_->setPlainText(s);
}

void ScriptItemWidget::infoProgress(VReply* reply)
{
    QString s=QString::fromStdString(reply->text());
    textEdit_->setPlainText(s);
}

void ScriptItemWidget::infoFailed(VReply* reply)
{
    QString s=QString::fromStdString(reply->errorText());
    textEdit_->setPlainText(s);
}


static InfoPanelItemMaker<ScriptItemWidget> maker1("script");