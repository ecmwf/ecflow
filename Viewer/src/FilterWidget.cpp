//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "FilterWidget.hpp"

#include <QDebug>
#include <QHBoxLayout>
#include <QPainter>
#include <QPixmap>
#include <QToolButton>

#include "VNState.hpp"
#include "VAttribute.hpp"
#include "VIcon.hpp"
#include "VFilter.hpp"

#include "ServerItem.hpp"
#include "ServerFilter.hpp"

//===========================================
//
// VParamFilterMenu
//
//===========================================

VParamFilterMenu::VParamFilterMenu(QMenu * parent,VParamSet* filter,DecorMode decorMode) :
 	menu_(parent),
	filter_(filter),
	decorMode_(decorMode)
{
	//Param name must be unique
	for(std::set<VParam*>::const_iterator it=filter_->all().begin(); it != filter_->all().end(); ++it)
	{
		addAction((*it)->label(),
				  (*it)->name());
	}

	if(decorMode_ == ColourDecor)
	{
		Q_FOREACH(QAction* ac,menu_->actions())
		{
			if(!ac->isSeparator())
			{
				if(VNState* vs=VNState::find(ac->data().toString().toStdString()))
				{
					QPixmap pix(10,10);
					QPainter painter(&pix);
					pix.fill(vs->colour());
					painter.setPen(Qt::black);
					painter.drawRect(0,0,9,9);
					ac->setIcon(pix);
				}
			}
		}
	}

	QAction *ac = new QAction(this);
	ac->setSeparator(true);
	menu_->addAction(ac);

	ac = new QAction(this);
	ac->setText(tr("Select all"));
	menu_->addAction(ac);
	connect(ac,SIGNAL(triggered(bool)),
			this,SLOT(slotSelectAll(bool)));

	ac = new QAction(this);
	ac->setText(tr("Unselect all"));
	menu_->addAction(ac);
	connect(ac,SIGNAL(triggered(bool)),
			this,SLOT(slotUnselectAll(bool)));

	reload();
}

void VParamFilterMenu::addAction(QString name,QString id)
{
	QAction *ac = new QAction(this);
	ac->setText(name);
	ac->setData(id);
	ac->setCheckable(true);
	ac->setChecked(false);

	menu_->addAction(ac);

	//It will not be emitted when setChecked is called!
    connect(ac,SIGNAL(triggered(bool)),
					this,SLOT(slotChanged(bool)));
}

void VParamFilterMenu::slotSelectAll(bool)
{
	Q_FOREACH(QAction* ac,menu_->actions())
	{
		if(!ac->isSeparator() &&
			ac->isCheckable())
		{
			ac->setChecked(true);
		}
	}

	slotChanged(true);
}

void VParamFilterMenu::slotUnselectAll(bool)
{
	Q_FOREACH(QAction* ac,menu_->actions())
	{
		if(!ac->isSeparator() &&
			ac->isCheckable())
		{
			ac->setChecked(false);
		}
	}

	slotChanged(true);
}

void VParamFilterMenu::slotChanged(bool)
{
	std::set<std::string> items;
	Q_FOREACH(QAction* ac,menu_->actions())
	{
		if(!ac->isSeparator() &&
			ac->isCheckable() && ac->isChecked())
		{
			items.insert(ac->data().toString().toStdString());
		}
	}

	if(filter_)
		filter_->current(items);
}

void VParamFilterMenu::reload()
{
	Q_FOREACH(QAction* ac,menu_->actions())
	{
		if(!ac->isSeparator())
		{
			ac->setChecked(filter_->isSet(ac->data().toString().toStdString()));
		}
	}
}


/*
//===========================================
//
// StateFilterMenu
//
//===========================================

StateFilterMenu::StateFilterMenu(QMenu * parent) :
		VParamFilterMenu(parent,VNState::filterItems())
{
	Q_FOREACH(QAction* ac,menu_->actions())
	{
		if(!ac->isSeparator())
		{
			if(VNState* vs=VNState::find(ac->data().toString().toStdString()))
			{
				QPixmap pix(10,10);
				QPainter painter(&pix);
				pix.fill(vs->colour());
				painter.setPen(Qt::black);
				painter.drawRect(0,0,9,9);
				ac->setIcon(pix);
			}
		}
	}
}

//===========================================
//
// AttributeFilterMenu
//
//===========================================

AttributeFilterMenu::AttributeFilterMenu(QMenu * parent) :
		VParamFilterMenu(parent,VAttribute::filterItems())
{
}

//===========================================
//
// IconFilterMenu
//
//===========================================

IconFilterMenu::IconFilterMenu(QMenu * parent) :
		VParamFilterMenu(parent,VIcon::filterItems())
{
}
*/


//===========================================
//
// ServerFilterMenu
//
//===========================================

ServerFilterMenu::ServerFilterMenu(QMenu * parent) :
 	menu_(parent),
	filter_(NULL)
{
	init();

	ServerList::instance()->addObserver(this);
}

ServerFilterMenu::~ServerFilterMenu()
{
	ServerList::instance()->removeObserver(this);
	if(filter_)
		filter_->removeObserver(this);
}

void ServerFilterMenu::clear()
{
	Q_FOREACH(QAction* ac,acLst_)
	{
		delete ac;
	}
	acLst_.clear();
}

void ServerFilterMenu::init()
{
	for(int i=0; i < ServerList::instance()->count(); i++)
	{
		ServerItem* item=ServerList::instance()->itemAt(i);
		//QString name=QString::fromStdString(item->name() + " (" + item->host() + ":" + item->port() + ")");
		QString name=QString::fromStdString(item->name());
		addAction(name,i);
	}
}

void ServerFilterMenu::addAction(QString name,int id)
{
	QAction *ac = new QAction(this);
	ac->setText(name);
	ac->setData(id);
	ac->setCheckable(true);
	ac->setChecked(false);

	menu_->addAction(ac);

	//It will not be emitted when setChecked is called!!
    connect(ac,SIGNAL(triggered(bool)),
					this,SLOT(slotChanged(bool)));

    acLst_ << ac;

}

void ServerFilterMenu::slotChanged(bool)
{
	if(!filter_)
		return;

	if(QAction *ac=static_cast<QAction*>(sender()))
	{
		if(ac->isSeparator()) return;

		if(ServerItem *item=ServerList::instance()->itemAt(ac->data().toInt()))
		{
			if(ac->isChecked())
				filter_->addServer(item);
			else
				filter_->removeServer(item);
		}
	}
}

//Reset actions state when a new filter is loaded
void ServerFilterMenu::reload(ServerFilter *filter)
{
	if(filter_)
		filter_->removeObserver(this);

	filter_=filter;

	if(!filter_)
		return;

	if(filter_)
	{
			filter_->addObserver(this);
			reload();
	}

}

//Reset actions state when a new filter is loaded
void ServerFilterMenu::reload()
{
	if(!filter_)
		return;

	Q_FOREACH(QAction* ac,acLst_)
	{
		if(!ac->isSeparator())
		{
				if(ServerItem *item=ServerList::instance()->itemAt(ac->data().toInt()))
				{
					//Triggered() will not be called!!
					ac->setChecked(filter_->isFiltered(item));
				}
		}
	}
}


void ServerFilterMenu::notifyServerListChanged()
{
	clear();
	init();
	reload();
}


void ServerFilterMenu::notifyServerFilterAdded(ServerItem*)
{
	reload();
}

void ServerFilterMenu::notifyServerFilterRemoved(ServerItem*)
{
	reload();
}

void ServerFilterMenu::notifyServerFilterChanged(ServerItem*)
{
	reload();
}





FilterWidget::FilterWidget(QWidget *parent) :
   QWidget(parent),
   data_(0)
{
	/*QHBoxLayout *hb=new QHBoxLayout();
	setLayout(hb);

	hb->setContentsMargins(0,0,0,0);
	hb->setSpacing(0);

	QList<DState::State> lst;
	lst << DState::UNKNOWN << DState::SUSPENDED << DState::COMPLETE << DState::QUEUED << DState::SUBMITTED << DState::ACTIVE << DState::ABORTED;

	Q_FOREACH(DState::State st,lst)
	{
		QToolButton* tb=createButton(ViewConfig::Instance()->stateShortName(st),ViewConfig::Instance()->stateName(st),ViewConfig::Instance()->stateColour(st));

		items_[st]=tb;
		//states_[]
		hb->addWidget(tb);

		//It will not be emitted when setChecked is called!
		connect(tb,SIGNAL(clicked(bool)),
				this,SLOT(slotChanged(bool)));

	}*/
}

QToolButton* FilterWidget::createButton(QString label,QString tooltip,QColor col)
{
	QToolButton *tb=new QToolButton(this);
	tb->setCheckable(true);
	tb->setChecked(true);
	tb->setText(label);
	tb->setToolTip(tooltip);

	QString s; //="QToolButton {border-radius: 0px;  padding: 2px;; border: black;}";

	s+="QToolButton::checked {background: rgb(" +
			QString::number(col.red()) + "," +
			QString::number(col.green()) + "," +
			QString::number(col.blue()) + ");}";
	tb->setStyleSheet(s);

	return tb;
}

void FilterWidget::reload(VParamSet* filterData)
{
	data_=filterData;

	/*const StateFilter& filter=data_->stateFilter();

	QMapIterator<DState::State,QToolButton*> it(items_);
	while (it.hasNext())
	{
		it.next();
		if(filter.find(it.key()) != filter.end())
			it.value()->setChecked(true);
		else
			it.value()->setChecked(false);
	}*/
}

void FilterWidget::slotChanged(bool)
{
	/*StateFilter filter;
	QMapIterator<DState::State,QToolButton*> it(items_);
	while (it.hasNext())
	{
	     it.next();
	     if(it.value()->isChecked())
	    		 filter.insert(it.key());
	}

	if(data_)
			data_->stateFilter(filter);*/
}
