/***************************** LICENSE START ***********************************

 Copyright 2012 ECMWF and INPE. This software is distributed under the terms
 of the Apache License version 2.0. In applying this license, ECMWF does not
 waive the privileges and immunities granted to it by virtue of its status as
 an Intergovernmental Organization or submit itself to any jurisdiction.

 ***************************** LICENSE END *************************************/

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
