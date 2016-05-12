//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================


#include "TextEditSearchLine.hpp"

#include "AbstractTextEditSearchInterface.hpp"

#include <QColor>
#include <QDebug>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>

//#include "UserMessage.hpp"

TextEditSearchLine::TextEditSearchLine(QWidget *parent) :
	AbstractSearchLine(parent),
	interface_(0)
{
	connect(matchModeCb_,SIGNAL(currentIndexChanged(int)),
		this, SLOT(matchModeChanged(int)));
}

TextEditSearchLine::~TextEditSearchLine()
{

}

void TextEditSearchLine::setSearchInterface(AbstractTextEditSearchInterface *e)
{
	interface_=e;
}

bool TextEditSearchLine::findString (QString str, bool highlightAll, QTextDocument::FindFlags extraFlags, bool gotoStartOfWord, int iteration)
{
	QTextDocument::FindFlags flags = findFlags() | extraFlags;
	return interface_->findString(str,highlightAll,flags,gotoStartOfWord,iteration,matchModeCb_->currentMatchMode());
}

void TextEditSearchLine::highlightMatches(QString txt)
{
	if(interface_)
	{
		interface_->enableHighlights();
		if(interface_->highlightsNeedSearch() && !txt.isEmpty())
		{
			findString(txt, true,  0, true, 0);   // highlight all matches
		}
	}
}

void TextEditSearchLine::slotHighlight()
{
	//UserMessage::message(UserMessage::DBG, false," highlight: " + searchLine_->text().toStdString());

	highlightAllTimer_.stop();

	if (highlightAll())
		highlightMatches(searchLine_->text());
}

//This slot is called as we type in the search string
void TextEditSearchLine::slotFind(QString txt)
{
	if(!interface_)
		return;

	//In confirmSearch mode we do not start the search
	if(confirmSearch_)
	{
		toDefaultState();
		return;
	}

	if(txt.isEmpty())
	{
		highlightAllTimer_.stop();
		toDefaultState();
		return;
	}

	highlightAllTimer_.stop();
	bool found = findString(txt, false, 0, true, 0);  // find the next match

	if (!isEmpty()) // there is a search term supplied by the user
	{
		// don't highlight the matches immediately - this can be expensive for large files,
		// and we don't want to highlight each time the user types a new character; wait
		// a moment and then start the highlight
		highlightAllTimer_.setInterval(500);
		highlightAllTimer_.disconnect();
		connect(&highlightAllTimer_, SIGNAL(timeout()), this, SLOT(slotHighlight()));
		highlightAllTimer_.start();
	}
	else
	{
		clearHighlights();
	}

	updateButtons(found);
}

void TextEditSearchLine::slotFindNext()
{
	if(!interface_)
		return;

	bool found = findString(searchLine_->text(), false, 0, false, 0);
	updateButtons(found);
}

void TextEditSearchLine::slotFindPrev()
{
	if(!interface_)
		return;

	bool found = findString(searchLine_->text(), false, QTextDocument::FindBackward, false, 0);
	updateButtons(found);
}

QTextDocument::FindFlags TextEditSearchLine::findFlags()
{
	QTextDocument::FindFlags flags;

	if(caseSensitive())
	{
		flags = flags | QTextDocument::FindCaseSensitively;
	}

	if(wholeWords())
	{
		flags = flags | QTextDocument::FindWholeWords;
	}

	return flags;
}


// EditorSearchLine::refreshSearch
// performed when the user changes search parameters such as case sensitivity - we want to
// re-do the search from the current point, but if the current selection still matches then
// we'd like it to be found first.

void TextEditSearchLine::refreshSearch()
{
	if(!interface_)
		return;

	// if there's something selected already then move the cursor to the start of the line and search again
	interface_->refreshSearch();

	slotFindNext();
	slotHighlight();
}

void TextEditSearchLine::disableHighlights()
{
    if(interface_)
        interface_->disableHighlights();
}


void TextEditSearchLine::clearHighlights()
{
    if(interface_)
        interface_->clearHighlights();
}

void TextEditSearchLine::matchModeChanged(int notUsed)
{
	if(matchModeCb_->currentMatchMode() == StringMatchMode::ContainsMatch)
		actionWholeWords_->setEnabled(true);
	else
		actionWholeWords_->setEnabled(false);

	refreshSearch();
}


void TextEditSearchLine::on_actionCaseSensitive__toggled(bool b)
{
    AbstractSearchLine::on_actionCaseSensitive__toggled(b);

	refreshSearch();
}


void TextEditSearchLine::on_actionWholeWords__toggled(bool b)
{
    AbstractSearchLine::on_actionWholeWords__toggled(b);

	refreshSearch();
}

void TextEditSearchLine::on_actionHighlightAll__toggled(bool b)
{
    AbstractSearchLine::on_actionHighlightAll__toggled(b);

	if (b)                  // user switched on the highlights
		slotHighlight();
	else                    // user switched off the highlights
        disableHighlights();

	if(interface_ && interface_->highlightsNeedSearch())
		refreshSearch();
}

void TextEditSearchLine::slotClose()
{
	AbstractSearchLine::slotClose();    
    clearHighlights();
}

// Called when we load a new node's information into the panel, or
// when we move to the panel from another one.
// If the search box is open, then search for the first matching item;
// otherwise, search for a pre-configured list of keywords. If none
// are found, and the user has clicked on the 'reload' button then
// we just go to the last line of the output
void TextEditSearchLine::searchOnReload(bool userClickedReload)
{
	if (isVisible() && !isEmpty())
	{
		slotFindNext();
		slotHighlight();
	}
	else if(interface_)
	{
		// search for a highlight any of the pre-defined keywords so that
		// the (probably) most important piece of information is highlighted
		interface_->automaticSearchForKeywords(userClickedReload);
	}
}

