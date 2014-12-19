//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "OverviewItemWidget.hpp"
#include "Highlighter.hpp"
#include "OverviewProvider.hpp"
#include "VReply.hpp"

//========================================================
//
// InfoItemWidget
//
//========================================================

OverviewItemWidget::OverviewItemWidget(QWidget *parent) : TextItemWidget(parent)
{
	QFont f;
	f.setFamily("Monospace");
	//f.setFamily("Courier");
	f.setStyleHint(QFont::TypeWriter);
	f.setFixedPitch(true);
	textEdit_->setFont(f);

	Highlighter* ih=new Highlighter(textEdit_->document(),"info");

	infoProvider_=new OverviewProvider(this);
}

QWidget* OverviewItemWidget::realWidget()
{
	return this;
}

void OverviewItemWidget::reload(VInfo_ptr nodeInfo)
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

void OverviewItemWidget::clearContents()
{
	loaded_=false;
	textEdit_->clear();
}

void OverviewItemWidget::infoReady(VReply* reply)
{
	QString s=QString::fromStdString(reply->text());
	textEdit_->setPlainText(s);
}

void OverviewItemWidget::infoProgress(VReply* reply)
{
	QString s=QString::fromStdString(reply->text());
	textEdit_->setPlainText(s);
}

void OverviewItemWidget::infoFailed(VReply* reply)
{
	QString s=QString::fromStdString(reply->errorText());
	textEdit_->setPlainText(s);
}


static InfoPanelItemMaker<OverviewItemWidget> maker1("overview");
