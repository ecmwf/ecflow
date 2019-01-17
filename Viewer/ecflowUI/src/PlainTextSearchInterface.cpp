//============================================================================
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "PlainTextSearchInterface.hpp"

#include <QPlainTextEdit>


PlainTextSearchInterface::PlainTextSearchInterface() : editor_(NULL)
{
}


bool PlainTextSearchInterface::findString (QString str, bool highlightAll, QTextDocument::FindFlags flags,
										   QTextCursor::MoveOperation move, int iteration,StringMatchMode::Mode matchMode)
{
	if(!editor_)
		return false;

    if(editor_->document()->isEmpty())
    {
        return false;
    }

	QTextCursor cursor(editor_->textCursor());

	if (highlightAll)  // if highlighting all matches, start from the start of the document
		cursor.movePosition(QTextCursor::Start);

	else // move the cursor?
		cursor.movePosition(move);


	QList<QTextEdit::ExtraSelection> extraSelections;
	bool found = false;
	bool keepGoing = true;
	int  numMatches = 0;

	Qt::CaseSensitivity cs = (flags & QTextDocument::FindCaseSensitively) ? Qt::CaseSensitive : Qt::CaseInsensitive;

	while (keepGoing)
	{
		switch (matchMode)
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
				if (flags & QTextDocument::FindBackward)
					cursor.movePosition(QTextCursor::End);
				else
					cursor.movePosition(QTextCursor::Start);
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

void PlainTextSearchInterface::automaticSearchForKeywords(bool userClickedReload)
{
    if(editor_->document()->isEmpty())
        return;

    bool performSearch = vpPerformAutomaticSearch_->value().toBool();

	if (performSearch)
	{
		// search direction
		QTextDocument::FindFlags findFlags;
		QTextCursor cursor(editor_->textCursor());
		std::string searchFrom = vpAutomaticSearchFrom_->valueAsStdString();
		QTextCursor::MoveOperation move;
		if (searchFrom == "bottom")
		{
			findFlags = QTextDocument::FindBackward;
			move = QTextCursor::End;
		}
		else
		{
			move = QTextCursor::Start;
		}

		// case sensitivity
		bool caseSensitive = vpAutomaticSearchCase_->value().toBool();
		if (caseSensitive)
			findFlags = findFlags | QTextDocument::FindCaseSensitively;

		// string match mode
		std::string matchMode(vpAutomaticSearchMode_->valueAsStdString());
		StringMatchMode::Mode mode = StringMatchMode::operToMode(matchMode);

		// the term to be searched for
		std::string searchTerm_s(vpAutomaticSearchText_->valueAsStdString());
		QString searchTerm = QString::fromStdString(searchTerm_s);

		// perform the search
		bool found = findString (searchTerm, false, findFlags, move, 1, mode);

		if(!found)
		{
			if(userClickedReload)
			{
				// move the cursor to the start of the last line
				gotoLastLine();
			}
		}
	}
	else
	{
		// move the cursor to the start of the last line
		gotoLastLine();
	}
}

void PlainTextSearchInterface::refreshSearch()
{
	if(!editor_)
		return;

	QTextCursor cursor(editor_->textCursor());
	if (cursor.hasSelection())
	{
		cursor.movePosition(QTextCursor::StartOfLine, QTextCursor::MoveAnchor);
		editor_->setTextCursor(cursor);
	}
}


void PlainTextSearchInterface::clearHighlights()
{
	if(!editor_)
		return;

	QList<QTextEdit::ExtraSelection> empty;
	editor_->setExtraSelections(empty);
}

void PlainTextSearchInterface::disableHighlights()
{
    clearHighlights();
}


void PlainTextSearchInterface::gotoLastLine()
{
	// move the cursor to the start of the last line
	QTextCursor cursor = editor_->textCursor();
	cursor.movePosition(QTextCursor::End);
	cursor.movePosition(QTextCursor::StartOfLine);
	editor_->setTextCursor(cursor);
	editor_->ensureCursorVisible();
}
