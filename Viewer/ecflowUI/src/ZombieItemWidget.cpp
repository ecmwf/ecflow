//============================================================================
// Copyright 2009-2017 ECMWF.
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

    connect(zombieView,SIGNAL(doubleClicked(const QModelIndex&)),
            this,SLOT(slotDoubleClicked(const QModelIndex&)));

	//Build context menu
    QAction* sep1=new QAction(this);
    sep1->setSeparator(true);

    zombieView->addAction(actionRescue);
    zombieView->addAction(actionFoboff);
	zombieView->addAction(actionKill);
	zombieView->addAction(actionTerminate);
    zombieView->addAction(actionDelete);
    zombieView->addAction(sep1);
    zombieView->addAction(actionLookup);

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

    if(info_ && info_->server())
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
    if(info_ && info_->server())
	{
		infoProvider_->info(info_);
	}
}

void ZombieItemWidget::updateState(const FlagSet<ChangeFlag>&)
{
    checkActionState();
}


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

void ZombieItemWidget::on_actionLookup_triggered()
{
    lookup(zombieView->currentIndex());
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
            QList<Zombie> zLst;
            Q_FOREACH(QModelIndex idx,lst)
            {
                zLst << model_->indexToZombie(sortModel_->mapToSource(idx));
            }

            Q_FOREACH(Zombie z,zLst)
            {
                if(z.empty() == false)
                {
                    VTask_ptr t=VTask::create(VTask::ZombieCommandTask);
                    t->setZombie(z);
                    t->param("command",cmdName);
                    info_->server()->run(t);
                    commandSent_=true;
                }
            }
		}
	}
}


void ZombieItemWidget::lookup(const QModelIndex& index)
{
    if(!info_ || !info_->server())
        return;

    QModelIndex idx=sortModel_->mapToSource(index);

    if(idx.isValid())
    {
        Zombie z=model_->indexToZombie(idx);
        std::string p=z.path_to_task();
        if(!p.empty())
        {
            VInfo_ptr ni=VInfo::createFromPath(info_->server(),p);
            if(ni)
            {
                InfoPanelItem::linkSelected(ni);
            }
        }
    }
}


void ZombieItemWidget::slotDoubleClicked(const QModelIndex &index)
{
    lookup(index);
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

bool ZombieItemWidget::hasSameContents(VInfo_ptr info)
{
    if(info && info_ && info->server())
    {
        return info->server() == info_->server();
    }
    return false;
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

