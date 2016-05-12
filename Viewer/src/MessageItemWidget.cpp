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

static bool firstRun=true;

MessageItemWidget::MessageItemWidget(QWidget *parent) : QWidget(parent)
{
    setupUi(this);

    searchTb_->setEnabled(false);
    searchTb_->setVisible(false);
    searchLine_->setVisible(false);

    infoProvider_=new MessageProvider(this);

    model_=new LogModel(this);

    treeView_->setProperty("log","1");
    treeView_->setModel(model_);
    treeView_->setItemDelegate(new LogDelegate(this));

    syncTb_->hide();
}

QWidget* MessageItemWidget::realWidget()
{
    return this;
}

void MessageItemWidget::reload(VInfo_ptr info)
{
    assert(active_);

    if(suspended_)
        return;

    clearContents();
    info_=info;

    if(info_)
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

    //Adjust column size if it is the first run
    if(firstRun && model_->hasData())
    {
    	firstRun=false;
    	for(int i=0; i < model_->columnCount()-1; i++)
    	{
            treeView_->resizeColumnToContents(i);
    	}
    }
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
