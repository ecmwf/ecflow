//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "MessageItemWidget.hpp"

#include "InfoProvider.hpp"
#include "VReply.hpp"

#include "LogModel.hpp"

MessageItemWidget::MessageItemWidget(QWidget *parent) : QWidget(parent)
{
    setupUi(this);

    searchLine_->setVisible(false);

    infoProvider_=new MessageProvider(this);

    model_=new LogModel(this);
    treeView_->setModel(model_);
}

QWidget* MessageItemWidget::realWidget()
{
    return this;
}

void MessageItemWidget::reload(VInfo_ptr info)
{
	clearContents();

	loaded_=true;
    info_=info;

    if(info_ && info_.get())
    {
        infoProvider_->info(info_);
    }
}

void MessageItemWidget::clearContents()
{
    InfoPanelItem::clear();

    model_->clearData();
}

void MessageItemWidget::infoReady(VReply* reply)
{
    model_->setData(reply->textVec());
}

void MessageItemWidget::infoProgress(VReply* reply)
{
    QString s=QString::fromStdString(reply->text());
}

void MessageItemWidget::infoFailed(VReply* reply)
{
    QString s=QString::fromStdString(reply->errorText());
}

static InfoPanelItemMaker<MessageItemWidget> maker1("message");
