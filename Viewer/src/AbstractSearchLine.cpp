//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include <QtGlobal>
#include <QShortcut>
#include <QMenu>
#include "AbstractSearchLine.hpp"

AbstractSearchLine::AbstractSearchLine(QWidget* parent) : QWidget(parent)
{
	setupUi(this);


#if QT_VERSION >= QT_VERSION_CHECK(4, 7, 0)
	searchLine_->setPlaceholderText(tr("Find"));
	label_->hide();
#endif

#if QT_VERSION >= QT_VERSION_CHECK(5, 2, 0)
	searchLine_->setClearButtonEnabled(true);
#endif

	connect(searchLine_, SIGNAL(textChanged(QString)),
		this, SLOT(slotFind(QString)));

	connect(searchLine_, SIGNAL( returnPressed()),
		this, SLOT(slotFindNext()));

	connect(actionNext_,SIGNAL(triggered()),
		this, SLOT(slotFindNext()));

	connect(actionPrev_,SIGNAL(triggered()),
		this, SLOT(slotFindPrev()));

	connect(closeTb_,SIGNAL(clicked()),
		this, SLOT(slotClose()));

	connect(matchModeCb_,SIGNAL(currentIndexChanged(int)),
		this, SLOT(matchModeChanged(int)));

	nextTb_->setDefaultAction(actionNext_);
	prevTb_->setDefaultAction(actionPrev_);

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
	optionsTb_->setMenu(menu);

    matchModeCb_->setMatchMode(StringMatchMode::ContainsMatch);  // set the default match mode
    //matchModeChanged(1);  // dummy call to initialise the 'whole words' option state
}

AbstractSearchLine::~AbstractSearchLine()
{
}

void AbstractSearchLine::clear()
{
	searchLine_->clear();
}

bool AbstractSearchLine::isEmpty()
{
	return searchLine_->text().isEmpty();
}

void AbstractSearchLine::selectAll()
{
	searchLine_->selectAll();
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

void AbstractSearchLine::slotClose()
{
	hide();
}

void AbstractSearchLine::on_actionCaseSensitive__toggled(bool b)
{
    caseSensitive_ = b;
}

void AbstractSearchLine::on_actionWholeWords__toggled(bool b)
{
    wholeWords_ = b;
}

void AbstractSearchLine::matchModeChanged(int notUsed)
{
    if(matchModeCb_->currentMatchMode() == StringMatchMode::ContainsMatch)
        actionWholeWords_->setEnabled(true);
    else
        actionWholeWords_->setEnabled(false);
}
