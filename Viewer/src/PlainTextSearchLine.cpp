//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include <QColor>
#include <QDebug>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>

#include "PlainTextSearchLine.hpp"
#include "VConfig.hpp"
#include "VProperty.hpp"
//#include "UserMessage.hpp"

PlainTextSearchLine::PlainTextSearchLine(QWidget *parent) :
	AbstractSearchLine(parent),
	editor_(0),
	highlightColour_(QColor(200, 255, 200))
{
	connect(matchModeCb_,SIGNAL(currentIndexChanged(int)),
		this, SLOT(matchModeChanged(int)));

	if(VProperty *p=VConfig::instance()->find("panel.search.highlightColour"))
	{
		highlightColour_=p->value().value<QColor>();
	}
}

PlainTextSearchLine::~PlainTextSearchLine()
{

}

void PlainTextSearchLine::setEditor(QPlainTextEdit *e)
{
	editor_=e;
}


bool PlainTextSearchLine::findString (QString str, bool highlightAll, QTextDocument::FindFlags extraFlags, bool gotoStartOfWord, int iteration)
{
	QTextDocument::FindFlags flags = findFlags() | extraFlags;

	QTextCursor cursor(editor_->textCursor());

	if (highlightAll)  // if highlighting all matches, start from the start of the document
		cursor.movePosition(QTextCursor::Start);

	else if (gotoStartOfWord)	// go to start of word?
		cursor.movePosition(QTextCursor::StartOfWord);


	QList<QTextEdit::ExtraSelection> extraSelections;
	bool found = false;
	bool keepGoing = true;
	int  numMatches = 0;

	Qt::CaseSensitivity cs = (flags & QTextDocument::FindCaseSensitively) ? Qt::CaseSensitive : Qt::CaseInsensitive;

	while (keepGoing)
	{
		switch (matchModeCb_->currentMatchMode())
		{
			case StringMatchMode::ContainsMatch:
			{
				cursor = editor_->document()->find(str, cursor, flags);  // perform the search
				found = (!cursor.isNull());

				break;
			}
			case StringMatchMode::WildcardMatch:
			{
				QRegExp regexp(str);
				regexp.setCaseSensitivity(cs);
				regexp.setPatternSyntax(QRegExp::Wildcard);

				cursor = editor_->document()->find(regexp, cursor, flags);  // perform the search
				found = (!cursor.isNull());

				break;
			}
			case StringMatchMode::RegexpMatch:
			{
				QRegExp regexp(str);
				regexp.setCaseSensitivity(cs);

				cursor = editor_->document()->find(regexp, cursor, flags);  // perform the search
				found = (!cursor.isNull());

				break;
			}

			default:
			{
				break;
			}
		}


		if (found)
		{
			if (highlightAll)
			{
				QTextEdit::ExtraSelection highlight;
				highlight.cursor = cursor;
				highlight.format.setBackground(highlightColour_);
				extraSelections << highlight;
				numMatches++;
			}
			else
			{
				editor_->setTextCursor(cursor);  // mark the selection of the match
			}
		}


		if (found && !highlightAll)  // found a match and we only want one - stop here and select it
			keepGoing = false;

		else if (!found && !highlightAll && (iteration != 0))  // didn't find a match, only want one, we HAVE wrapped around
			keepGoing = false;

		if (!found && highlightAll)  // want to highlight all, but no more matches found
			keepGoing = false;



		// not found, and we only want one match, then we need to wrap around and keep going
		if (keepGoing)
		{
			if (!highlightAll)
			{
				cursor=editor_->textCursor();
				if (extraFlags & QTextDocument::FindBackward)
					cursor.movePosition(QTextCursor::End);
				else
					cursor.movePosition(QTextCursor::Start);
				editor_->setTextCursor(cursor);
				iteration = 1;  // iteration=1 to avoid infinite wraparound!
			}
		}
	}


	if (highlightAll)
	{
		//char num[64];
		//sprintf(num, "%d", numMatches);
		//UserMessage::message(UserMessage::DBG, false," highlighting : " + std::string(num));

		editor_->setExtraSelections( extraSelections );
	}

	return (found);
}



void PlainTextSearchLine::highlightMatches(QString txt)
{
	if (!txt.isEmpty())
		findString(txt, true,  0, true, 0);   // highlight all matches
}


void PlainTextSearchLine::slotHighlight()
{
	//UserMessage::message(UserMessage::DBG, false," highlight: " + searchLine_->text().toStdString());

	highlightAllTimer_.stop();

	if (highlightAll())
		highlightMatches(searchLine_->text());
}


void PlainTextSearchLine::slotFind(QString txt)
{
	if(!editor_)
		return;

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

void PlainTextSearchLine::slotFindNext()
{
	if(!editor_)
		return;

	bool found = findString(searchLine_->text(), false, 0, false, 0);
	updateButtons(found);
}

void PlainTextSearchLine::slotFindPrev()
{
	if(!editor_)
		return;

	bool found = findString(searchLine_->text(), false, QTextDocument::FindBackward, false, 0);
	updateButtons(found);
}

QTextDocument::FindFlags PlainTextSearchLine::findFlags()
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





// PlainTextSearchLine::refreshSearch
// performed when the user changes search parameters such as case sensitivity - we want to
// re-do the search from the current point, but if the current selection still matches then
// we'd like it to be found first.

void PlainTextSearchLine::refreshSearch()
{
	// if there's something selected already then move the cursor to the start of the line and search again
	QTextCursor cursor(editor_->textCursor());
	if (cursor.hasSelection())
	{
		cursor.movePosition(QTextCursor::StartOfLine, QTextCursor::MoveAnchor);
		editor_->setTextCursor(cursor);
	}
	slotFindNext();
	slotHighlight();
}


void PlainTextSearchLine::clearHighlights()
{
	QList<QTextEdit::ExtraSelection> empty;
	editor_->setExtraSelections(empty);
}


void PlainTextSearchLine::matchModeChanged(int notUsed)
{
	if(matchModeCb_->currentMatchMode() == StringMatchMode::ContainsMatch)
		actionWholeWords_->setEnabled(true);
	else
		actionWholeWords_->setEnabled(false);

	refreshSearch();
}


void PlainTextSearchLine::on_actionCaseSensitive__toggled(bool b)
{
    AbstractSearchLine::on_actionCaseSensitive__toggled(b);

	refreshSearch();
}


void PlainTextSearchLine::on_actionWholeWords__toggled(bool b)
{
    AbstractSearchLine::on_actionWholeWords__toggled(b);

	refreshSearch();
}

void PlainTextSearchLine::on_actionHighlightAll__toggled(bool b)
{
    AbstractSearchLine::on_actionHighlightAll__toggled(b);

	if (b)                  // user switched on the highlights
		slotHighlight();
	else                    // user switched off the highlights
		clearHighlights();


	refreshSearch();
}

void PlainTextSearchLine::slotClose()
{
	AbstractSearchLine::slotClose();
	clearHighlights();
}

