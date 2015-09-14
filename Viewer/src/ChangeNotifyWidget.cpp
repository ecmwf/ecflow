//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "ChangeNotifyWidget.hpp"

#include <QHBoxLayout>
#include <QSignalMapper>
#include <QToolButton>

#include "ChangeNotify.hpp"
#include "VNodeList.hpp"
#include "VProperty.hpp"

#include <vector>

std::vector<ChangeNotifyWidget*> ChangeNotifyWidget::widgets_;

ChangeNotifyButton::ChangeNotifyButton(QWidget* parent) :
	QToolButton(parent),
	notifier_(0)
{
	setAutoRaise(true);
}

void ChangeNotifyButton::setNotifier(ChangeNotify* notifier)
{
	notifier_=notifier;

	setText(QString::fromStdString(notifier_->id()));

	connect(this,SIGNAL(clicked(bool)),
			this,SLOT(slotClicked(bool)));

	connect(notifier_->data(),SIGNAL(endAppendRow()),
			this,SLOT(slotAppend()));

	connect(notifier_->data(),SIGNAL((endReset())),
				this,SLOT(slotReset()));
}

void ChangeNotifyButton::slotAppend()
{
}

void ChangeNotifyButton::slotReset()
{
}

void ChangeNotifyButton::slotClicked(bool)
{
	ChangeNotify::showDialog(notifier_->id());
}


ChangeNotifyWidget::ChangeNotifyWidget(QWidget *parent) : QWidget(parent)
{
	layout_=new QHBoxLayout(this);

	ChangeNotify::populate(this);

	widgets_.push_back(this);
}

ChangeNotifyWidget::~ChangeNotifyWidget()
{
	std::vector<ChangeNotifyWidget*>::iterator it=std::find(widgets_.begin(),widgets_.end(),this);
	if(it != widgets_.end())
		widgets_.erase(it);
}

void ChangeNotifyWidget::addTb(ChangeNotify* notifier)
{
	ChangeNotifyButton *tb=new ChangeNotifyButton(this);
	tb->setNotifier(notifier);
	layout_->addWidget(tb);
}

/*
void ChangeNotifyWidget::update(const std::string& id)
{
	std::map<std::string,QToolButton*>::iterator it=tbMap_.find(id)
	if(it != tbMap_.end())
	{
		QToolButton *tb=it->second;

		ChangeNotify* n=ChangeNotify::find(id)

		if(it != widgets_.end())
}
*/
/*
void ChangeNotifyWidget::changed(const std::string& id)
{
	for(std::vector<ChangeNotifyWidget*>::iterator it=widgets_.begin(); it != widgets_.end(); it++)
	{
		(*it)->update(id);
	}

	//ids <<  id;
}
*/
