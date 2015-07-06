//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "AbstractSearchLine.hpp"

AbstractSearchLine::AbstractSearchLine(QWidget* parent) : QWidget(parent)
{
	setupUi(this);

	//searchLine_->setDecoration(QPixmap(":/viewer/filter_decor.svg"));

	connect(searchLine_, SIGNAL(textChanged(QString)),
		this, SLOT(slotFind(QString)));

	connect(searchLine_, SIGNAL( returnPressed()),
		this, SLOT(slotFindNext()));

	connect(nextPb_, SIGNAL(clicked()),
		this, SLOT(slotFindNext()));

	connect(prevPb_,SIGNAL(clicked()),
		this, SLOT(slotFindPrev()));

	oriColour_=QColor(searchLine_->palette().color(QPalette::Base));
	redColour_=QColor(247,230,230);
	greenColour_=QColor(186,249,206);

	status_=true;

	setFocusProxy(searchLine_);
}

AbstractSearchLine::~AbstractSearchLine()
{
	clear();
}

void AbstractSearchLine::clear()
{
	searchLine_->clear();
}

void AbstractSearchLine::updateButtons(bool found)
{
	status_=found;

	if(searchLine_->text().isEmpty())
	{
	  	QPalette p=searchLine_->palette();
		p.setColor(QPalette::Base,oriColour_);
		searchLine_->setPalette(p);
	}
	else
	{
		if(!found)
		{
			QPalette p=searchLine_->palette();
			p.setColor(QPalette::Base,redColour_);
			searchLine_->setPalette(p);
		}
		else
		{
			QPalette p=searchLine_->palette();
			p.setColor(QPalette::Base,greenColour_);
			searchLine_->setPalette(p);
		}
	}
}
