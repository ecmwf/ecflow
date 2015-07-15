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
#include "VReply.hpp"

HistoryItemWidget::HistoryItemWidget(QWidget *parent) : CodeItemWidget(parent)
{
	fileLabel_->hide();
	textEdit_->setShowLineNumbers(false);

	infoProvider_=new HistoryProvider(this);
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
    textEdit_->clear();
}

void HistoryItemWidget::infoReady(VReply* reply)
{
    QString s=QString::fromStdString(reply->text());
    textEdit_->setPlainText(s);
}

void HistoryItemWidget::infoProgress(VReply* reply)
{
    QString s=QString::fromStdString(reply->text());
    textEdit_->setPlainText(s);
}

void HistoryItemWidget::infoFailed(VReply* reply)
{
    QString s=QString::fromStdString(reply->errorText());
    textEdit_->setPlainText(s);
}

static InfoPanelItemMaker<HistoryItemWidget> maker1("history");
