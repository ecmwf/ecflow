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
#include <QToolButton>

#include "VState.hpp"
#include "VAttribute.hpp"
#include "VIcon.hpp"
#include "VFilter.hpp"

//===========================================
//
// AbstractFilterMenu
//
//===========================================

AbstractFilterMenu::AbstractFilterMenu(QMenu * parent,const std::vector<VParam*>& pars) :
 	menu_(parent),
	filter_(NULL)
{
	for(std::vector<VParam*>::const_iterator it=pars.begin(); it != pars.end(); it++)
	{
		addAction((*it)->text("label"),VParam::toInt((*it)->type()));
	}
}

void AbstractFilterMenu::addAction(QString name,int id)
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

void AbstractFilterMenu::slotChanged(bool)
{
	std::set<VParam::Type> items;
	foreach(QAction* ac,menu_->actions())
	{
		if(!ac->isSeparator())
		{
			if(ac->isChecked())
					items.insert(VParam::toType(ac->data().toInt()));
		}
	}

	if(filter_)
		filter_->current(items);
}

void AbstractFilterMenu::reload(VFilter* filter)
{
	filter_=filter;
	foreach(QAction* ac,menu_->actions())
	{
		if(!ac->isSeparator())
		{
			VParam::Type type=VParam::toType(ac->data().toInt());
			ac->setChecked(filter_->isSet(type));
		}
	}
}



//===========================================
//
// StateFilterMenu
//
//===========================================

StateFilterMenu::StateFilterMenu(QMenu * parent) :
		AbstractFilterMenu(parent,VState::filterItems())
{
}

//===========================================
//
// AttributeFilterMenu
//
//===========================================

AttributeFilterMenu::AttributeFilterMenu(QMenu * parent) :
		AbstractFilterMenu(parent,VAttribute::filterItems())
{
}

//===========================================
//
// IconFilterMenu
//
//===========================================

IconFilterMenu::IconFilterMenu(QMenu * parent) :
		AbstractFilterMenu(parent,VIcon::filterItems())
{
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

	foreach(DState::State st,lst)
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

void FilterWidget::reload(VFilter* filterData)
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
