//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

// Copyright 2014 ECMWF.

#include "TextPager/TextPagerSearchInterface.hpp"

//#include <QTextEdit>
#include "TextPagerEdit.hpp"
#include "UserMessage.hpp"

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
#include <QGuiApplication>
#endif

bool TextPagerSearchInterface::findString (QString str, bool highlightAll, QTextDocument::FindFlags flags,
		                                bool gotoStartOfWord, int iteration,StringMatchMode::Mode matchMode)
{
	if(!editor_)
		return false;

	bool doSearch=true;
	if(str.simplified().isEmpty())
	{
		doSearch=false;
	}

	TextPagerCursor cursor(editor_->textCursor());

	if (gotoStartOfWord)	// go to start of word?
		cursor.movePosition(TextPagerCursor::StartOfWord);

	TextPagerDocument::FindMode mode=TextPagerDocument::FindWrap;

	if(flags & QTextDocument::FindCaseSensitively)
		mode |= TextPagerDocument::FindCaseSensitively;

	if(flags & QTextDocument::FindBackward)
		mode |= TextPagerDocument::FindBackward;

	if(flags & QTextDocument::FindWholeWords)
		mode |= TextPagerDocument::FindWholeWords;

	bool found = false;

	Qt::CaseSensitivity cs = (flags & QTextDocument::FindCaseSensitively) ? Qt::CaseSensitive : Qt::CaseInsensitive;

	if(doSearch)
	{
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
		QGuiApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
#endif
	}

	switch (matchMode)
	{
		case StringMatchMode::ContainsMatch:
		{
			editor_->setSearchHighlighter(str,mode);

			if(doSearch)
			{
				cursor = editor_->document()->find(str, cursor, mode);  // perform the search
				found = (!cursor.isNull());
			}
			break;
		}
		case StringMatchMode::WildcardMatch:
		{
			QRegExp regexp(str);
			regexp.setCaseSensitivity(cs);
			regexp.setPatternSyntax(QRegExp::Wildcard);

			editor_->setSearchHighlighter(regexp,mode);

			if(doSearch)
			{
				cursor = editor_->document()->find(regexp, cursor, mode);  // perform the search			}
				found = (!cursor.isNull());
			}
			break;
		}
		case StringMatchMode::RegexpMatch:
		{
			QRegExp regexp(str);
			regexp.setCaseSensitivity(cs);

			editor_->setSearchHighlighter(regexp,mode);

			if(doSearch)
			{
				cursor = editor_->document()->find(regexp, cursor, mode);  // perform the search
				found = (!cursor.isNull());
			}
			break;
		}

		default:
			break;
	}

	if(found)
	{
		editor_->setTextCursor(cursor);  // mark the selection of the match
	}

	if(doSearch)
	{
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
		QGuiApplication::restoreOverrideCursor();
#endif
	}

	return found;
}

void TextPagerSearchInterface::automaticSearchForKeywords(bool userClickedReload)
{
	bool found = false;

	TextPagerDocument::FindMode findMode = TextPagerDocument::FindBackward;
	TextPagerCursor cursor(editor_->textCursor());
	cursor.movePosition(TextPagerCursor::End);

    //qDebug() << "automaticSearchForKeyword" << editor_->textCursor().position();

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
	QGuiApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
#endif

	//QRegExp regexp("10:56:45 4");
	QRegExp regexp("--(abort|complete)");
	TextPagerCursor findCursor = editor_->document()->find(regexp, cursor, findMode);  // perform the search
	//TextPagerCursor findCursor = editor_->document()->find("--abort", cursor, findMode);
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
			TextPagerCursor cursor = editor_->textCursor();
			cursor.movePosition(TextPagerCursor::End);
			cursor.movePosition(TextPagerCursor::StartOfLine);
			editor_->setTextCursor(cursor);
		}
	}

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
	QGuiApplication::restoreOverrideCursor();
#endif
}

void TextPagerSearchInterface::refreshSearch()
{
	if(!editor_)
		return;

	TextPagerCursor cursor(editor_->textCursor());
	if (cursor.hasSelection())
	{
		cursor.movePosition(TextPagerCursor::StartOfLine, TextPagerCursor::MoveAnchor);
		editor_->setTextCursor(cursor);
	}
}


void TextPagerSearchInterface::clearHighlights()
{
	if(!editor_)
		return;

	editor_->setEnableSearchHighlighter(false);


	/*QList<TextPagerEdit::ExtraSelection> empty;
	editor_->setExtraSelections(empty);*/
}

void TextPagerSearchInterface::enableHighlights()
{
	if(!editor_)
		return;

	editor_->setEnableSearchHighlighter(true);
}



