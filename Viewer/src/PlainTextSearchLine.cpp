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

void PlainTextSearchLine::slotFind(QString txt)
{
	if(!editor_)
		return;

	QTextCursor cursor(editor_->textCursor());
	cursor.movePosition(QTextCursor::StartOfWord);
	editor_->setTextCursor(cursor);

	if(editor_->find(txt)==false)
	{
      	cursor=editor_->textCursor();
		cursor.movePosition(QTextCursor::Start);
		editor_->setTextCursor(cursor);
		updateButtons(editor_->find(txt));
	}
	else
	{
		updateButtons(true);
	}
}

void PlainTextSearchLine::slotFindNext()
{
	if(!editor_)
			return;

	if(status_==true)
	{
		if(editor_->find(searchLine_->text()) == false)
		{
      			QTextCursor cursor(editor_->textCursor());
      			cursor.movePosition(QTextCursor::Start);
			editor_->setTextCursor(cursor);
			editor_->find(searchLine_->text());
		}
	}
}

void PlainTextSearchLine::slotFindPrev()
{
	if(!editor_)
			return;

	QTextDocument::FindFlags flags=QTextDocument::FindBackward;

	if(status_==true)
	{
		if(editor_->find(searchLine_->text(),flags) == false)
		{
      			QTextCursor cursor(editor_->textCursor());
      			cursor.movePosition(QTextCursor::End);
			editor_->setTextCursor(cursor);
			editor_->find(searchLine_->text(),flags);
		}
	}
}
