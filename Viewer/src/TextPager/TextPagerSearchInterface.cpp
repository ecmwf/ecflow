//============================================================================
// Copyright 2016 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================
#include <assert.h>

#include "TextPager/TextPagerSearchInterface.hpp"

#include "TextPagerEdit.hpp"
#include "UserMessage.hpp"

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
#include <QGuiApplication>
#endif

TextPagerCursor::MoveOperation TextPagerSearchInterface::translateCursorMoveOp(QTextCursor::MoveOperation move)
{
	switch (move)
	{
		case QTextCursor::NoMove:       return TextPagerCursor::NoMove;
		case QTextCursor::Start:        return TextPagerCursor::Start;
		case QTextCursor::StartOfLine:  return TextPagerCursor::StartOfLine;
		case QTextCursor::StartOfWord:  return TextPagerCursor::StartOfWord;
		case QTextCursor::PreviousWord: return TextPagerCursor::PreviousWord;
		case QTextCursor::End:          return TextPagerCursor::End;
		default:
		{
			assert(0);
			return TextPagerCursor::NoMove;
		}
	}
}


bool TextPagerSearchInterface::findString (QString str, bool highlightAll, QTextDocument::FindFlags flags,
										   QTextCursor::MoveOperation move, int iteration, StringMatchMode::Mode matchMode)
{
	if(!editor_)
		return false;

    if(editor_->document()->documentSize() == 0)
        return false;

    bool doSearch=true;
	if(str.simplified().isEmpty())
	{
		doSearch=false;
	}

	TextPagerCursor cursor(editor_->textCursor());

	cursor.movePosition(translateCursorMoveOp(move));  // move the cursor?

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
        editor_->ensureCursorVisible();
        cursor.movePosition(TextPagerCursor::StartOfLine);
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
    if(editor_->document()->documentSize() ==0)
        return;

    bool performSearch = vpPerformAutomaticSearch_->value().toBool();

	if (performSearch)
	{

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
		QGuiApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
#endif

		// search direction
		QTextDocument::FindFlags findFlags;
		TextPagerCursor cursor(editor_->textCursor());
		std::string searchFrom = vpAutomaticSearchFrom_->valueAsString();
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
		std::string matchMode(vpAutomaticSearchMode_->valueAsString());
		StringMatchMode::Mode mode = StringMatchMode::operToMode(matchMode);

		// the term to be searched for
		std::string searchTerm_s(vpAutomaticSearchText_->valueAsString());
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

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
		QGuiApplication::restoreOverrideCursor();
#endif

	}
	else
	{
		// move the cursor to the start of the last line
		gotoLastLine();
	}
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
    if(editor_)
        editor_->clearSearchHighlighter();
}

void TextPagerSearchInterface::enableHighlights()
{
    if(editor_)
        editor_->setEnableSearchHighlighter(true);
}

void TextPagerSearchInterface::disableHighlights()
{
    if(editor_)
        editor_->setEnableSearchHighlighter(false);
}

void TextPagerSearchInterface::gotoLastLine()
{
	// move the cursor to the start of the last line
	TextPagerCursor cursor = editor_->textCursor();
	cursor.movePosition(TextPagerCursor::End);
	cursor.movePosition(TextPagerCursor::StartOfLine);
	editor_->setTextCursor(cursor);
	editor_->ensureCursorVisible();
}
