//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "HistoryItemWidget.hpp"

#include "InfoProvider.hpp"
#include "LogModel.hpp"
#include "VReply.hpp"

HistoryItemWidget::HistoryItemWidget(QWidget *parent) : QWidget(parent)
{
	setupUi(this);

	fileLabel_->hide();

	infoProvider_=new HistoryProvider(this);

	model_=new LogModel(this);
	treeView_->setModel(model_);
}

QWidget* HistoryItemWidget::realWidget()
{
    return this;
}

void HistoryItemWidget::reload(VInfo_ptr info)
{
	clearContents();

	loaded_=true;
    info_=info;

    if(info_ && info_.get())
    {
        infoProvider_->info(info_);
    }
}

void HistoryItemWidget::clearContents()
{
    InfoPanelItem::clear();
    model_->clearData();
}

void HistoryItemWidget::infoReady(VReply* reply)
{
    //QString s=QString::fromStdString(reply->text());
    model_->setData(reply->text());
}

void HistoryItemWidget::infoProgress(VReply* reply)
{
    QString s=QString::fromStdString(reply->text());
}

void HistoryItemWidget::infoFailed(VReply* reply)
{
    QString s=QString::fromStdString(reply->errorText());
}

static InfoPanelItemMaker<HistoryItemWidget> maker1("history");
