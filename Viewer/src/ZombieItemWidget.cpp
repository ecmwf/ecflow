//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "ZombieItemWidget.hpp"

#include <QItemSelectionModel>
#include <QSortFilterProxyModel>

#include "ServerHandler.hpp"
#include "VNode.hpp"
#include "VReply.hpp"
#include "InfoProvider.hpp"
#include "ZombieModel.hpp"

static bool firstRun=true;

//========================================================
//
// ZombieItemWidget
//
//========================================================

ZombieItemWidget::ZombieItemWidget(QWidget *parent) :
	QWidget(parent),
	commandSent_(false)
{
	setupUi(this);

	infoProvider_=new ZombieProvider(this);

	model_=new ZombieModel(this);

	sortModel_=new QSortFilterProxyModel(this);
	sortModel_->setSourceModel(model_);

	zombieView->setModel(sortModel_);

	//The selection changes in the view
	connect(zombieView->selectionModel(),SIGNAL(currentChanged(QModelIndex,QModelIndex)),
			this,SLOT(slotItemSelected(QModelIndex,QModelIndex)));

	//Build context menu
	zombieView->addAction(actionRescue);
	zombieView->addAction(actionFoboff);
	zombieView->addAction(actionKill);
	zombieView->addAction(actionTerminate);
	zombieView->addAction(actionDelete);

	//Add actions for the pushbuttons
	terminateTb_->setDefaultAction(actionTerminate);
	rescueTb_->setDefaultAction(actionRescue);
	foboffTb_->setDefaultAction(actionFoboff);
	deleteTb_->setDefaultAction(actionDelete);
	killTb_->setDefaultAction(actionKill);

	checkActionState();
}

ZombieItemWidget::~ZombieItemWidget()
{

}

QWidget* ZombieItemWidget::realWidget()
{
	return this;
}

void ZombieItemWidget::reload(VInfo_ptr info)
{
    assert(active_);

    if(suspended_)
        return;

    clearContents();
	info_=info;

    if(info_ && info_->isServer() && info_->server())
	{
		commandSent_=false;
		infoProvider_->info(info_);
	}
}

void ZombieItemWidget::clearContents()
{
	InfoPanelItem::clear();
	model_->clearData();
	checkActionState();
}


void ZombieItemWidget::updateContents()
{
	//model_->clearData();
    if(info_ && info_->isServer() && info_->server())
	{
		infoProvider_->info(info_);
	}
}

void ZombieItemWidget::updateState(const FlagSet<ChangeFlag>&)
{
    checkActionState();
}

/*
void ZombieItemWidget::saveSelection()
{
	QModelIndexList lst=zombieView->selectionModel()->selectedRows(0);
	lastSelection_.clear();

	if(!lst.isEmpty())
	{
		std::vector<std::string> paths;
		Q_FOREACH(QModelIndex idx,lst)
		{
			lastSelection_.push_back(model_->data(idx,Qt::DisplayRole).toString().toStdString());
		}
	}
}

void ZombieItemWidget::resetSelection()
{
	Q_FOREACH(QString s,lastSelection_)
	{


			QModelIndex idx,lst)
	{
		lastSelection_.push_back(model_->data(idx,Qt::DisplayRole).toString().toStdString());
	}


	QModelIndexList lst=zombieView->selectionModel()->selectedRows(0);
	lastSelection_.clear();

	if(!lst.isEmpty())
	{
		std::vector<std::string> paths;
		Q_FOREACH(QModelIndex idx,lst)
		{
			lastSelection_.push_back(model_->data(idx,Qt::DisplayRole).toString().toStdString());
		}
	}
}
*/


void ZombieItemWidget::infoReady(VReply* reply)
{
	commandSent_=false;

	//We need to know what task it was!
	if(model_->hasData())
	{
		model_->updateData(reply->zombies());
	}
	else
	{
		model_->setData(reply->zombies());
	}

	//Adjust column size if it is the first run
	if(firstRun && model_->hasData())
	{
		firstRun=false;
		for(int i=0; i < model_->columnCount()-1; i++)
		{
			zombieView->resizeColumnToContents(i);
		}
	}
	checkActionState();
}

void ZombieItemWidget::infoFailed(VReply* reply)
{
	commandSent_=false;
	//QString s=QString::fromStdString(reply->errorText());
	checkActionState();
}

void ZombieItemWidget::on_actionTerminate_triggered()
{
	command("zombie_fail");
}

void ZombieItemWidget::on_actionRescue_triggered()
{
	command("zombie_adopt");
}

void ZombieItemWidget::on_actionFoboff_triggered()
{
	command("zombie_fob");
}

void ZombieItemWidget::on_actionDelete_triggered()
{
	command("zombie_remove");
}

void ZombieItemWidget::on_actionKill_triggered()
{
	command("zombie_kill");
}

void ZombieItemWidget::on_reloadTb__clicked(bool)
{
	updateContents();
}

void ZombieItemWidget::command(const std::string& cmdName)
{
	if(info_ && info_->server())
	{
		//Get selection form view
        QModelIndexList lst=zombieView->selectionModel()->selectedRows(0);

		if(!lst.isEmpty())
		{
			std::vector<std::string> paths;
			Q_FOREACH(QModelIndex idx,lst)
			{
				paths.push_back(model_->data(sortModel_->mapToSource(idx),Qt::DisplayRole).toString().toStdString());
			}

			std::vector<std::string> cmd;
			cmd.push_back("ecflow_client");
			cmd.push_back("--" + cmdName);
			cmd.push_back("<full_name>");

			commandSent_=true;

			info_->server()->command(paths,cmd);
		}
	}
}


void ZombieItemWidget::slotItemSelected(QModelIndex,QModelIndex)
{
	checkActionState();
}

void ZombieItemWidget::checkActionState()
{
    if(suspended_)
    {
        reloadTb_->setEnabled(false);
        actionRescue->setEnabled(false);
        actionFoboff->setEnabled(false);
        actionKill->setEnabled(false);
        actionTerminate->setEnabled(false);
        actionDelete->setEnabled(false);
        return;
    }
    else
    {
        reloadTb_->setEnabled(true);
    }

    QModelIndex vIndex=zombieView->currentIndex();
	QModelIndex index=sortModel_->mapToSource(vIndex);

	bool acState=(index.isValid())?true:false;

	actionRescue->setEnabled(acState);
	actionFoboff->setEnabled(acState);
	actionKill->setEnabled(acState);
	actionTerminate->setEnabled(acState);
	actionDelete->setEnabled(acState);
}

void ZombieItemWidget::serverSyncFinished()
{
	//If a command has previously sent
	if(commandSent_)
	{
		commandSent_=false;
		updateContents();
	}
}


static InfoPanelItemMaker<ZombieItemWidget> maker1("zombie");

