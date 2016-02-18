//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "PlainTextSearchInterface.hpp"

#include <QPlainTextEdit>


bool PlainTextSearchInterface::findString (QString str, bool highlightAll, QTextDocument::FindFlags flags,
		                                bool gotoStartOfWord, int iteration,StringMatchMode::Mode matchMode)
{
	if(!editor_)
		return false;

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

void PlainTextSearchInterface::automaticSearchForKeywords(bool userClickedReload)
{
	bool found = false;

	QTextDocument::FindFlags findFlags = QTextDocument::FindBackward;
	QTextCursor cursor(editor_->textCursor());
	cursor.movePosition(QTextCursor::End);

	QRegExp regexp("--(abort|complete)");
	QTextCursor findCursor = editor_->document()->find(regexp, cursor, findFlags);  // perform the search
	found = (!findCursor.isNull());
	if(found)
	{
		editor_->setTextCursor(findCursor);
	}

#if 0
	QStringList keywords;
	keywords << "--abort" << "--complete";// << "xabort" << "xcomplete"
	         << "System Billing Units";

	// find any of the keywords and stop at the first one
	int i = 0;
	while (!found && i < keywords.size())
	{
		cursor.movePosition(QTextCursor::End);
		textEdit_->setTextCursor(cursor);
		found = textEdit_->findString(keywords.at(i), findFlags);
		i++;
	}
#endif

	else
	{
		if(userClickedReload)
		{
			// move the cursor to the start of the last line
			QTextCursor cursor = editor_->textCursor();
			cursor.movePosition(QTextCursor::End);
			cursor.movePosition(QTextCursor::StartOfLine);
			editor_->setTextCursor(cursor);
		}
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
