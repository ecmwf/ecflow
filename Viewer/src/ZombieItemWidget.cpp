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

//========================================================
//
// ZombieItemWidget
//
//========================================================

ZombieItemWidget::ZombieItemWidget(QWidget *parent) : QWidget(parent)
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
	zombieView->addAction(actionTerminate);
	zombieView->addAction(actionRescue);
	zombieView->addAction(actionFoboff);
	zombieView->addAction(actionDelete);
	zombieView->addAction(actionKill);

	//Add actions for the pushbuttons
	terminateTb_->setDefaultAction(actionTerminate);
	rescueTb_->setDefaultAction(actionRescue);
	foboffTb_->setDefaultAction(actionFoboff);
	deleteTb_->setDefaultAction(actionDelete);
	killTb_->setDefaultAction(actionKill);
}

QWidget* ZombieItemWidget::realWidget()
{
	return this;
}

void ZombieItemWidget::reload(VInfo_ptr info)
{
	clearContents();

	loaded_=true;
	info_=info;

	if(info_.get() && info_->isServer() && info_->server())
	{
		infoProvider_->info(info_);
	}
}

void ZombieItemWidget::clearContents()
{
	InfoPanelItem::clear();
	model_->clearData();
}

void ZombieItemWidget::infoReady(VReply* reply)
{
	//We need to know what task it was!
	model_->setData(reply->zombies());
}

void ZombieItemWidget::infoFailed(VReply* reply)
{
    QString s=QString::fromStdString(reply->errorText());
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
				paths.push_back(model_->data(idx,Qt::DisplayRole).toString().toStdString());
			}

			std::vector<std::string> cmd;
			cmd.push_back("ecflow_client");
			cmd.push_back("--" + cmdName);
			cmd.push_back("<full_path>");

			info_->server()->command(paths,cmd,false);
		}
	}
}

static InfoPanelItemMaker<ZombieItemWidget> maker1("zombie");

