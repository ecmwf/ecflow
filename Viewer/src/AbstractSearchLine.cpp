//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include <QShortcut>
#include <QMenu>
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



	// for the 'find next' functionality, although Qt (at the time of writing) uses
	// both F3 and CTRL-G for most platforms, this is not true for Linux. Therefore,
	// we have to add CTRL-G ourselves.

	QKeySequence ctrlg(tr("Ctrl+G"));
    QShortcut *shortcut = new QShortcut(ctrlg, parent); // should be destroyed by parent
    connect(shortcut, SIGNAL(activated()), this, SLOT(slotFindNext()));

	status_=true;


	setFocusProxy(searchLine_);



	// set the menu on the Options toolbutton
	caseSensitive_ = false;
	wholeWords_    = false;
	QMenu *menu=new QMenu(this);
	menu->addAction(actionCaseSensitive_);
	menu->addAction(actionWholeWords_);
	optionsPb_->setMenu(menu);


}

AbstractSearchLine::~AbstractSearchLine()
{
	clear();
}

void AbstractSearchLine::clear()
{
	searchLine_->clear();
}

bool AbstractSearchLine::isEmpty()
{
	return searchLine_->text().isEmpty();
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

void AbstractSearchLine::on_actionCaseSensitive__toggled(bool b)
{
    caseSensitive_ = b;
}

void AbstractSearchLine::on_actionWholeWords__toggled(bool b)
{
    wholeWords_ = b;
}
