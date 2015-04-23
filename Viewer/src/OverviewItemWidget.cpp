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

#include <QScrollBar>


//========================================================
//
// InfoItemWidget
//
//========================================================

OverviewItemWidget::OverviewItemWidget(QWidget *parent) :
  TextItemWidget(parent),
  lastScrollPos_(0)
{
	QFont f;
	f.setFamily("Monospace");
	//f.setFamily("Courier");
	f.setStyleHint(QFont::TypeWriter);
	f.setFixedPitch(true);
	textEdit_->setFont(f);
	textEdit_->setShowLineNumbers(false);

	Highlighter* ih=new Highlighter(textEdit_->document(),"info");

	infoProvider_=new OverviewProvider(this);
}

QWidget* OverviewItemWidget::realWidget()
{
	return this;
}

void OverviewItemWidget::reload(VInfo_ptr info)
{
	//set the info
	adjust(info);

	if(!info_.get())
	{
		textEdit_->clear();
	}
	else
	{
		clearContents();
		infoProvider_->info(info_);
	}

	loaded_=true;
}

void OverviewItemWidget::reload()
{
	//Save the vertical scrollbar pos
	lastScrollPos_=textEdit_->verticalScrollBar()->value();

	textEdit_->clear();
	infoProvider_->info(info_);
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

	//Restore the vertical scrollbar pos
	textEdit_->verticalScrollBar()->setValue(lastScrollPos_);
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

//At this point we can be sure that the node is handled by this item.
void OverviewItemWidget::nodeChanged(const VNode* node, const std::vector<ecf::Aspect::Type>& aspect)
{
	for(std::vector<ecf::Aspect::Type>::const_iterator it=aspect.begin(); it != aspect.end(); it++)
	{
		if(*it == ecf::Aspect::STATE || *it == ecf::Aspect::ADD_REMOVE_NODE || *it == ecf::Aspect::ADD_REMOVE_ATTR ||
		   *it == ecf::Aspect::DEFSTATUS || *it == ecf::Aspect::SUSPENDED || *it == ecf::Aspect::NODE_VARIABLE)
		{
			reload();
			return;
		}
	}
}

void OverviewItemWidget::defsChanged(const std::vector<ecf::Aspect::Type>& aspect)
{
	for(std::vector<ecf::Aspect::Type>::const_iterator it=aspect.begin(); it != aspect.end(); it++)
	{
		if(*it == ecf::Aspect::SERVER_STATE || *it == ecf::Aspect::SERVER_VARIABLE || *it == ecf::Aspect::ADD_REMOVE_ATTR)
		{
			reload();
			return;
		}
	}
}

void OverviewItemWidget::connectStateChanged()
{
	reload();
}

static InfoPanelItemMaker<OverviewItemWidget> maker1("overview");
