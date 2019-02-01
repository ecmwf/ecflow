//============================================================================
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "OverviewItemWidget.hpp"
#include "Highlighter.hpp"
#include "OverviewProvider.hpp"
#include "VConfig.hpp"
#include "VReply.hpp"

#include <QScrollBar>


//========================================================
//
// InfoItemWidget
//
//========================================================

OverviewItemWidget::OverviewItemWidget(QWidget *parent) :
  CodeItemWidget(parent)
{
	fileLabel_->hide();
	externalTb_->hide();
    copyPathTb_->hide();
    messageLabel_->setShowTypeTitle(false);

	textEdit_->setShowLineNumbers(false);

    //The document becomes the owner of the highlighter
    new Highlighter(textEdit_->document(),"info");

	infoProvider_=new OverviewProvider(this);

	//Editor font
	textEdit_->setFontProperty(VConfig::instance()->find("panel.overview.font"));
}

OverviewItemWidget::~OverviewItemWidget()
= default;

QWidget* OverviewItemWidget::realWidget()
{
	return this;
}

void OverviewItemWidget::reload(VInfo_ptr info)
{
    assert(active_);

    if(suspended_)
        return;

    clearContents();

    //set the info
    adjust(info);

    //Info must be a node
    if(info_)
    {
        reloadTb_->setEnabled(false);
        infoProvider_->info(info_);
    }
}

void OverviewItemWidget::reload()
{
	//Save the vertical scrollbar pos
	lastScrollPos_=textEdit_->verticalScrollBar()->value();

	textEdit_->clear();
    reloadTb_->setEnabled(false);
    infoProvider_->info(info_);
}

void OverviewItemWidget::clearContents()
{
	InfoPanelItem::clear();
	textEdit_->clear();
    reloadTb_->setEnabled(true);
}

void OverviewItemWidget::infoReady(VReply* reply)
{
    Q_ASSERT(reply);
    QString s=QString::fromStdString(reply->text());
	textEdit_->setPlainText(s);

	//Restore the vertical scrollbar pos
	textEdit_->verticalScrollBar()->setValue(lastScrollPos_);

    reloadTb_->setEnabled(true);
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

    reloadTb_->setEnabled(true);
}

//At this point we can be sure that the node is handled by this item.
void OverviewItemWidget::nodeChanged(const VNode* node, const std::vector<ecf::Aspect::Type>& aspect)
{
	if(frozen_)
		return;

	for(std::vector<ecf::Aspect::Type>::const_iterator it=aspect.begin(); it != aspect.end(); ++it)
	{
        if(*it != ecf::Aspect::ORDER && *it != ecf::Aspect::NOT_DEFINED)
		{
			reload();
			return;
		}
	}
}

void OverviewItemWidget::defsChanged(const std::vector<ecf::Aspect::Type>& aspect)
{
	if(frozen_)
		return;

	for(auto it : aspect)
	{
		if(it == ecf::Aspect::SERVER_STATE || it == ecf::Aspect::SERVER_VARIABLE || it == ecf::Aspect::ADD_REMOVE_ATTR)
		{
			reload();
			return;
		}
	}
}

void OverviewItemWidget::connectStateChanged()
{
	if(frozen_)
			return;

	reload();
}


void OverviewItemWidget::reloadRequested()
{
    reload();
}

void OverviewItemWidget::updateState(const FlagSet<ChangeFlag>& flags)
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

static InfoPanelItemMaker<OverviewItemWidget> maker1("overview");
