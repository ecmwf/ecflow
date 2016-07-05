//============================================================================
// Copyright 2016 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "SuiteItemWidget.hpp"

#include "InfoProvider.hpp"
#include "ServerHandler.hpp"
#include "SuiteFilter.hpp"
#include "SuiteModel.hpp"
#include "VNode.hpp"
#include "VReply.hpp"

//========================================================
//
// SuiteItemWidget
//
//========================================================

SuiteItemWidget::SuiteItemWidget(QWidget *parent) :
    QWidget(parent),
    columnsAdjusted_(false)
{
	setupUi(this);

	infoProvider_=new SuiteProvider(this);

	model_=new SuiteModel(this);

	suiteView->setModel(model_);

	connect(model_,SIGNAL(dataChanged(QModelIndex,QModelIndex)),
			this,SLOT(slotModelEdited(QModelIndex,QModelIndex)));

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

    //QPalette pal=okTb->palette();
    //QColor col(230,245,253);
    //pal.setColor(QPalette::Button,col);
    //okTb->setPalette(pal);

    okTb->setEnabled(false);
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

    if(info_ && info_->isServer() && info_->server())
	{
		//Get the current suitefilter
		SuiteFilter *sf=info_->server()->suiteFilter();

		assert(sf);

		//The model will be an observer of the suitefilter
        model_->setData(sf,info_->server());

        if(!columnsAdjusted_)
        {
            for(int i=0; i < model_->columnCount()-1; i++)
            {
                suiteView->resizeColumnToContents(i);
                suiteView->setColumnWidth(i,suiteView->columnWidth(i)+(i==0)?25:15);
            }
            columnsAdjusted_=true;
        }

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
    model_->setData(0,0);
	InfoPanelItem::clear();
	okTb->setEnabled(false);
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
        return;
    }
    else
    {
         enableTb->setEnabled(true);
         suiteView->setEnabled(true);
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
		sf->setAutoAddNewSuites(val);
        settingsChanged();
	}
}

void SuiteItemWidget::on_enableTb_clicked(bool val)
{
	if(SuiteFilter* sf=model_->filter())
	{
		sf->setEnabled(val);
		model_->reloadData();
		settingsChanged();
	}

    checkActionState();
}

void SuiteItemWidget::on_selectAllTb_clicked(bool)
{
	if(SuiteFilter* sf=model_->filter())
	{
		sf->selectAll();
		model_->reloadData();
		settingsChanged();
	}
}

void SuiteItemWidget::on_unselectAllTb_clicked(bool)
{
	if(SuiteFilter* sf=model_->filter())
	{
		sf->unselectAll();
		model_->reloadData();
		settingsChanged();
	}
}

//get a fresh suite list from the server
void SuiteItemWidget::on_syncTb_clicked(bool)
{
	if(info_.get() && info_->isServer() && info_->server())
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
            model_->reloadData();
            settingsChanged();
        }
    }

    checkActionState();
}

void SuiteItemWidget::on_okTb_clicked(bool)
{
	if(info_.get() && info_->isServer() && info_->server())
	{
		//This replace the edited filter in model the one
		//stored by the server
		info_->server()->updateSuiteFilter(model_->filter());
        okTb->setEnabled(false);
        messageLabel->clear();
        messageLabel->hide();
	}
}

void SuiteItemWidget::slotModelEdited(const QModelIndex&,const QModelIndex&)
{
	settingsChanged();
}

void SuiteItemWidget::settingsChanged()
{
    SuiteFilter *sf=model_->filter();
    SuiteFilter *oriSf=model_->realFilter();
    if(sf && oriSf)
    {
        bool st=oriSf->sameAs(sf);
        okTb->setEnabled(!st);
        if(st)
        {
            messageLabel->clear();
            messageLabel->hide();
        }
        else
        {
            messageLabel->showInfo("The suite filter has changed! Please click <b>Apply</b> to submit the changes to the server!");
        }
    }
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
