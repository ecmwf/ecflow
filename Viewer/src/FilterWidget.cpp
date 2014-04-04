//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "FilterWidget.hpp"

#include <QHBoxLayout>
#include <QToolButton>

#include "ViewFilter.hpp"
#include "ViewConfig.hpp"

FilterWidget::FilterWidget(QWidget *parent) :
   QWidget(parent),
   data_(0)
{
	QHBoxLayout *hb=new QHBoxLayout();
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

	}
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

void FilterWidget::reload(ViewFilter* filterData)
{
	data_=filterData;

	const std::set<DState::State>& ns=data_->nodeState();

	QMapIterator<DState::State,QToolButton*> it(items_);
	while (it.hasNext())
	{
		it.next();
		if(ns.find(it.key()) != ns.end())
		{
			it.value()->setChecked(true);
		}
		else
		{
			it.value()->setChecked(false);
		}
	}
}

void FilterWidget::slotChanged(bool)
{
	std::set<DState::State> filter;
	QMapIterator<DState::State,QToolButton*> it(items_);
	while (it.hasNext())
	{
	     it.next();
	     if(it.value()->isChecked())
	    		 filter.insert(it.key());
	}

	if(data_)
			data_->setNodeState(filter);
}
