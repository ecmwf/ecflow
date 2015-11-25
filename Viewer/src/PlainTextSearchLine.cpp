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

PlainTextSearchLine::PlainTextSearchLine(QWidget *parent) :
	AbstractSearchLine(parent), editor_(0)
{

}

PlainTextSearchLine::~PlainTextSearchLine()
{

}

void PlainTextSearchLine::setEditor(QPlainTextEdit *e)
{
	editor_=e;
}


bool PlainTextSearchLine::findString (QString str, QTextDocument::FindFlags extraFlags, bool gotoStartOfWord, int iteration)
{
	QTextDocument::FindFlags flags = findFlags() | extraFlags;

	QTextCursor cursor(editor_->textCursor());

	if (gotoStartOfWord)	// go to start of word?
		cursor.movePosition(QTextCursor::StartOfWord);

	editor_->setTextCursor(cursor);

	bool found = false;

	switch (matchModeCb_->currentMatchMode())
	{
		case StringMatchMode::ContainsMatch:
		{
			found = editor_->find(str, flags);
			break;
		}
		case StringMatchMode::WildcardMatch:
		{
			QRegExp regexp(str);
			Qt::CaseSensitivity cs = (flags & QTextDocument::FindCaseSensitively) ? Qt::CaseSensitive : Qt::CaseInsensitive;
			regexp.setCaseSensitivity(cs);
			regexp.setPatternSyntax(QRegExp::Wildcard);

			cursor = editor_->document()->find(regexp, editor_->textCursor(), flags);  // perform the search

			if (!cursor.isNull())
				editor_->setTextCursor(cursor);  // mark the selection of the match

			found = (!cursor.isNull());
			break;
			break;
		}
		case StringMatchMode::RegexpMatch:
		{
			QRegExp regexp(str);
			Qt::CaseSensitivity cs = (flags & QTextDocument::FindCaseSensitively) ? Qt::CaseSensitive : Qt::CaseInsensitive;
			regexp.setCaseSensitivity(cs);

			cursor = editor_->document()->find(regexp, editor_->textCursor(), flags);  // perform the search

			if (!cursor.isNull())
				editor_->setTextCursor(cursor);  // mark the selection of the match

			found = (!cursor.isNull());
			break;
		}
		default:
		{
			break;
		}
	}

	// if not found, then go back to the top and try again (wraparound)
	if (!found && (iteration == 0))
	{
		cursor=editor_->textCursor();
		if (extraFlags & QTextDocument::FindBackward)
			cursor.movePosition(QTextCursor::End);
		else
			cursor.movePosition(QTextCursor::Start);
		editor_->setTextCursor(cursor);
		found = findString(str, extraFlags, gotoStartOfWord, 1); // iteration=1 to avoid infinite wraparound!
	}

	return (found);
}


void PlainTextSearchLine::slotFind(QString txt)
{
	if(!editor_)
		return;

	bool found = findString(txt, 0, true, 0);
	updateButtons(found);
}

void PlainTextSearchLine::slotFindNext()
{
	if(!editor_)
		return;

	if(status_ == true)
	{
		findString(searchLine_->text(), 0, false, 0);
	}
}

void PlainTextSearchLine::slotFindPrev()
{
	if(!editor_)
			return;

	if(status_==true)
	{
		findString(searchLine_->text(), QTextDocument::FindBackward, false, 0);
	}
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
