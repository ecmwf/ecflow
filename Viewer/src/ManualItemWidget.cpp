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
#include "VReply.hpp"


ManualItemWidget::ManualItemWidget(QWidget *parent) : CodeItemWidget(parent)
{
    QFont f;
    f.setFamily("Monospace");
    //f.setFamily("Courier");
    f.setStyleHint(QFont::TypeWriter);
    f.setFixedPitch(true);
    textEdit_->setFont(f);

    Highlighter* ih=new Highlighter(textEdit_->document(),"manual");

    infoProvider_=new ManualProvider(this);
  
}

QWidget* ManualItemWidget::realWidget()
{
    return this;
}

void ManualItemWidget::reload(VInfo_ptr nodeInfo)
{
    loaded_=true;
    info_=nodeInfo;

    if(!nodeInfo.get())
    {
        textEdit_->clear();
    }
    else
    {
        clearContents();
        infoProvider_->info(info_);
    }   
}

void ManualItemWidget::clearContents()
{
    loaded_=false;
    textEdit_->clear();
}

void ManualItemWidget::infoReady(VReply* reply)
{
    QString s=QString::fromStdString(reply->text());
    textEdit_->setPlainText(s);
}

void ManualItemWidget::infoProgress(VReply* reply)
{
    QString s=QString::fromStdString(reply->text());
    textEdit_->setPlainText(s);
}

void ManualItemWidget::infoFailed(VReply* reply)
{
    QString s=QString::fromStdString(reply->errorText());
    textEdit_->setPlainText(s);
}


static InfoPanelItemMaker<ManualItemWidget> maker1("manual");

