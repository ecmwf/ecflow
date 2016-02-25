//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "HistoryItemWidget.hpp"

#include "LogProvider.hpp"
#include "LogModel.hpp"
#include "VReply.hpp"

static bool firstRun=true;

HistoryItemWidget::HistoryItemWidget(QWidget *parent) : QWidget(parent)
{
	setupUi(this);

	searchTb_->setEnabled(false);
	searchTb_->setVisible(false);
	searchLine_->setVisible(false);

	infoProvider_=new LogProvider(this);
	infoProvider_->setAutoUpdate(true);

	model_=new LogModel(this);

	treeView_->setProperty("log","1");
	treeView_->setModel(model_);
	treeView_->setItemDelegate(new LogDelegate(this));

    checkActionState();
}

QWidget* HistoryItemWidget::realWidget()
{
    return this;
}

void HistoryItemWidget::reload(VInfo_ptr info)
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

void HistoryItemWidget::clearContents()
{
    InfoPanelItem::clear();
	model_->clearData();
}

void HistoryItemWidget::infoReady(VReply* reply)
{
    model_->setData(reply->text());
    adjustColumnSize();
    fileLabel_->update(reply,"(Last 100 lines)");
    treeView_->scrollTo(model_->lastIndex());
    checkActionState();
}

void HistoryItemWidget::infoProgress(VReply* reply)
{
    QString s=QString::fromStdString(reply->text());
}

void HistoryItemWidget::infoFailed(VReply* reply)
{
    QString s=QString::fromStdString(reply->errorText());

    checkActionState();
}

void HistoryItemWidget::infoAppended(VReply* reply)
{
    model_->appendData(reply->textVec());
    adjustColumnSize();
    treeView_->scrollTo(model_->lastIndex());

    checkActionState();
}

void HistoryItemWidget::updateState(const ChangeFlags& flags)
{
    if(flags.isSet(SelectedChanged))
    {
        if(!selected_)
        {
            infoProvider_->setAutoUpdate(false);
            reloadTb_->setEnabled(false);
            return;
        }
    }

    if(flags.isSet(SuspendedChanged))
    {
        //Suspend
        if(suspended_)
        {
            infoProvider_->setAutoUpdate(false);
            reloadTb_->setEnabled(false);
            return;
        }

    }

    checkActionState();
}

void HistoryItemWidget::checkActionState()
{
    if(suspended_)
        return;

    if(infoProvider_->autoUpdate() == frozen_)
	{
		infoProvider_->setAutoUpdate(!frozen_);
	}
	reloadTb_->setEnabled(!infoProvider_->autoUpdate() || !infoProvider_->inAutoUpdate());
}

//Adjust column size if it is the first run
void HistoryItemWidget::adjustColumnSize()
{
	if(firstRun && model_->hasData())
	{
	   firstRun=false;
	   for(int i=0; i < model_->columnCount()-1; i++)
	   {
		   treeView_->resizeColumnToContents(i);
	   }
	}
}

void HistoryItemWidget::on_reloadTb__clicked(bool)
{
	if(info_ && info_.get())
	{
		infoProvider_->info(info_);
	}
}


static InfoPanelItemMaker<HistoryItemWidget> maker1("history");
