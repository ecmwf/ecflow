//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "SuiteItemWidget.hpp"

#include <QSortFilterProxyModel>

#include "InfoProvider.hpp"
#include "ServerHandler.hpp"
#include "SuiteFilter.hpp"
#include "SuiteModel.hpp"
#include "VNode.hpp"
#include "VReply.hpp"
#include "UiLog.hpp"

#define SuiteItemWidget__DEBUG__

//========================================================
//
// SuiteItemWidget
//
//========================================================

//Interface to edit the suite filter.
//This class contains a model which itself contains a SuiteFilter object. This is a
//copy of the original SuiteFilter stored by ServerHandler. The idea is that the
//editing is performed on this copy then users need to click apply
//to update the original SuiteFilter. The complication comes from the fact that
//the original SuiteFilter can be modified independently from this interface
//(e.g. suites are added/loaded/removed from the server). Then we need to make sure
//that the changes are merged into the edited state.

SuiteItemWidget::SuiteItemWidget(QWidget *parent) :
    QWidget(parent)
{
	setupUi(this);

	infoProvider_=new SuiteProvider(this);

	model_=new SuiteModel(this);
    auto* sortModel=new QSortFilterProxyModel(this);
    sortModel->setSourceModel(model_);

    suiteView->setUniformRowHeights(true);
    suiteView->setSortingEnabled(true);
    suiteView->setModel(sortModel);

	connect(model_,SIGNAL(dataChanged(QModelIndex,QModelIndex)),
			this,SLOT(slotModelEdited(QModelIndex,QModelIndex)));

    connect(model_,SIGNAL(dataUpdated()),
            this,SLOT(slotModelDataUpdated()));

    messageLabel->setShowTypeTitle(false);
    messageLabel->setNarrowMode(true);
    messageLabel->hide();

    QFont labelF;
    labelF.setBold(true);
    labelF.setPointSize(labelF.pointSize()-1);

    controlLabel->setFont(labelF);
    controlLabel->setText("<font color=\'#565656\'>" + controlLabel->text() + "</font>");

    selectLabel->setFont(labelF);
    selectLabel->setText("<font color=\'#565656\'>" + selectLabel->text() + "</font>");

    loadedLabel->setFont(labelF);
    loadedLabel->setText("<font color=\'#565656\'>" + loadedLabel->text() + "</font>");

    QPalette pal=okTb->palette();
    QColor col(10,150,10);
    pal.setColor(QPalette::Active,QPalette::Button,col);
    pal.setColor(QPalette::Active,QPalette::ButtonText,QColor(Qt::white));
    //okTb->setPalette(pal);

    okTb->setEnabled(false);
    resetTb->setEnabled(false);
	enableTb->setChecked(false);

    checkActionState();

    okTb->setEnabled(false);
}

QWidget* SuiteItemWidget::realWidget()
{
	return this;
}

void SuiteItemWidget::reload(VInfo_ptr info)
{
    assert(active_);

    if(suspended_)
        return;

    clearContents();

	info_=info;

    if(info_ && info_->server())
	{
		//Get the current suitefilter
		SuiteFilter *sf=info_->server()->suiteFilter();

        UiLog().dbg() << "SuiteItemWidget::reload 1";
        sf->filter();

		assert(sf);

		//The model will be an observer of the suitefilter
        model_->setData(sf,info_->server());

        UiLog().dbg() << "SuiteItemWidget::reload 2";
        sf->filter();

        if(!columnsAdjusted_)
        {
            for(int i=0; i < model_->columnCount()-1; i++)
            {
                suiteView->resizeColumnToContents(i);
                suiteView->setColumnWidth(i,suiteView->columnWidth(i) + ((i==0)?25:15));
            }
            columnsAdjusted_=true;
        }

        suiteView->sortByColumn(0,Qt::AscendingOrder);

        model_->filter()->addObserver(this);

		enableTb->setChecked(sf->isEnabled());
		autoCb->setChecked(sf->autoAddNewSuites());

        checkActionState();

		//We update the filter because it might not show the current status. If
		//there is a change the model will be notified

		//If the filter is disabled we update the filter with the
		//current list of suites in the defs. These are all the suites
		//loaded to the server.
		if(!sf->isEnabled())
		{
			info_->server()->updateSuiteFilterWithDefs();
		}
		//If the filter is enabled we need to fetch the total list of suites
		//loaded onto the server directly from the server (through the thread)
		else
		{
			//inforReady or infoFailed will always be called.
			infoProvider_->info(info_);
		}

        UiLog().dbg() << "SuiteItemWidget::reload 3";
        sf->filter();
	}
}

void SuiteItemWidget::updateData()
{
	/*if(info_.get() && info_->isServer() && info_->server())
	{
		model_->updateData(info_->server()->suiteFilter());
	}*/
}

void SuiteItemWidget::infoReady(VReply* reply)
{
    suiteView->sortByColumn(0,Qt::AscendingOrder);
    //updateData();
}

void SuiteItemWidget::infoFailed(VReply* reply)
{
	//commandSent_=false;
	//QString s=QString::fromStdString(reply->errorText());
	//checkActionState();
}

void SuiteItemWidget::clearContents()
{
    model_->setData(nullptr,nullptr);
	InfoPanelItem::clear();
	okTb->setEnabled(false);
    resetTb->setEnabled(false);
    messageLabel->hide();
}

void SuiteItemWidget::updateState(const FlagSet<ChangeFlag>&)
{
    checkActionState();
}

void SuiteItemWidget::checkActionState()
{
    if(suspended_)
    {
        enableTb->setEnabled(false);
        autoCb->setEnabled(false);
        selectAllTb->setEnabled(false);
        unselectAllTb->setEnabled(false);
        syncTb->setEnabled(false);
        removeTb->setEnabled(false);
        suiteView->setEnabled(false);
        okTb->setEnabled(false);
        resetTb->setEnabled(false);
        messageLabel->clear();
        messageLabel->hide();
        return;
    }
    else
    {
         enableTb->setEnabled(true);
         suiteView->setEnabled(true);
         bool st=model_->isEdited();
         if(st)
         {
             SuiteFilter *sf=model_->filter();
             SuiteFilter *oriSf=model_->realFilter();
             if(sf && oriSf)
             {
                 st=(sf->sameAsLoadedIgnored(oriSf) == false);
             }
         }

         okTb->setEnabled(st);
         resetTb->setEnabled(st);
         if(!st)
         {
            messageLabel->clear();
            messageLabel->hide();
         }
         else
         {
            messageLabel->showTip("You have edited the suite filter! Please click <b>Apply</b> to submit the changes to the server!");
         }
    }

    if(enableTb->isChecked())
	{
		autoCb->setEnabled(true);
		selectAllTb->setEnabled(true);
        unselectAllTb->setEnabled(true);
        syncTb->setEnabled(true);

		if(SuiteFilter* sf=model_->filter())
		{
			autoCb->setChecked(sf->autoAddNewSuites());
            removeTb->setEnabled(sf->hasUnloaded());
            syncTb->setEnabled(true);

		}
	}
	else
	{
		autoCb->setEnabled(false);
		selectAllTb->setEnabled(false);
		unselectAllTb->setEnabled(false);
        syncTb->setEnabled(false);
        removeTb->setEnabled(false);
	}
}

void SuiteItemWidget::on_autoCb_clicked(bool val)
{
	if(SuiteFilter* sf=model_->filter())
	{
        model_->setEdited(true);
        sf->setAutoAddNewSuites(val);
        checkActionState();
	}
}

void SuiteItemWidget::on_enableTb_clicked(bool val)
{
	if(SuiteFilter* sf=model_->filter())
	{
        sf->setEnabled(val);
        model_->setEdited(true);
        model_->reloadData();
        checkActionState();
	}

    checkActionState();
}

void SuiteItemWidget::on_selectAllTb_clicked(bool)
{
	if(SuiteFilter* sf=model_->filter())
	{		
        sf->selectAll();
        model_->setEdited(true);
        model_->reloadData();
        checkActionState();
	}
}

void SuiteItemWidget::on_unselectAllTb_clicked(bool)
{
	if(SuiteFilter* sf=model_->filter())
	{
		sf->unselectAll();
        model_->setEdited(true);
        model_->reloadData();
        checkActionState();
	}
}

//get a fresh suite list from the server
void SuiteItemWidget::on_syncTb_clicked(bool)
{
    if(info_.get() && info_->server())
	{
		infoProvider_->info(info_);
	}
}

void SuiteItemWidget::on_removeTb_clicked(bool val)
{
    if(SuiteFilter* sf=model_->filter())
    {
        if(sf->removeUnloaded())
        {
            model_->setEdited(true);
            model_->reloadData();
            checkActionState();
        }
    }

    checkActionState();
}

void SuiteItemWidget::on_okTb_clicked(bool)
{
    if(info_.get() && info_->server())
	{
        //This replace the edited filter in the model with the one
		//stored by the server
        model_->setEdited(false);
        info_->server()->updateSuiteFilter(model_->filter());
        okTb->setEnabled(false);
        resetTb->setEnabled(false);
        messageLabel->clear();
        messageLabel->hide();
	}
}

void SuiteItemWidget::on_resetTb_clicked(bool)
{
    if(info_.get() && info_->server())
    {
        model_->setEdited(false);
        model_->resetData();
        okTb->setEnabled(false);
        resetTb->setEnabled(false);
        messageLabel->clear();
        messageLabel->hide();
    }
}


void SuiteItemWidget::slotModelEdited(const QModelIndex&,const QModelIndex&)
{
    checkActionState();
}

void SuiteItemWidget::slotModelDataUpdated()
{
    checkActionState();
}


void SuiteItemWidget::notifyChange(SuiteFilter *filter)
{
    checkActionState();
}

void SuiteItemWidget::notifyDelete(SuiteFilter *filter)
{
    Q_ASSERT(filter);
    filter->removeObserver(this);
}


static InfoPanelItemMaker<SuiteItemWidget> maker1("suite");
