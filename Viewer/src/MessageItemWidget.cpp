//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "MessageItemWidget.hpp"

#include "Highlighter.hpp"
#include "InfoProvider.hpp"
#include "VReply.hpp"

MessageItemWidget::MessageItemWidget(QWidget *parent) : TextItemWidget(parent)
{
    QFont f;
    f.setFamily("Monospace");
    //f.setFamily("Courier");
    f.setStyleHint(QFont::TypeWriter);
    f.setFixedPitch(true);
    textEdit_->setFont(f);
    textEdit_->setShowLineNumbers(false);

    Highlighter* ih=new Highlighter(textEdit_->document(),"message");

    infoProvider_=new MessageProvider(this);

}

QWidget* MessageItemWidget::realWidget()
{
    return this;
}

void MessageItemWidget::reload(VInfo_ptr nodeInfo)
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

void MessageItemWidget::clearContents()
{
    loaded_=false;
    textEdit_->clear();
}

void MessageItemWidget::infoReady(VReply* reply)
{
    QString s=QString::fromStdString(reply->text());
    textEdit_->setPlainText(s);
}

void MessageItemWidget::infoProgress(VReply* reply)
{
    QString s=QString::fromStdString(reply->text());
    textEdit_->setPlainText(s);
}

void MessageItemWidget::infoFailed(VReply* reply)
{
    QString s=QString::fromStdString(reply->errorText());
    textEdit_->setPlainText(s);
}


static InfoPanelItemMaker<MessageItemWidget> maker1("message");
