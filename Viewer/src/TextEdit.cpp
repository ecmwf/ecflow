//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "TextEdit.hpp"

#include <QDebug>
#include <QFile>
#include <QPainter>
#include <QTextBlock>

TextEdit::TextEdit(QWidget * parent) :
    QPlainTextEdit(parent),
    showLineNum_(true),
    rightMargin_(2)
{
    lineNumArea_ = new LineNumberArea(this);

    connect(this,SIGNAL(blockCountChanged(int)),
    		this,SLOT(updateLineNumberAreaWidth(int)));

    connect(this,SIGNAL(updateRequest(QRect,int)),
    		this,SLOT(updateLineNumberArea(QRect,int)));

    connect(this,SIGNAL(cursorPositionChanged()),
    		lineNumArea_,SLOT(update()));

    updateLineNumberAreaWidth(0);
}

TextEdit::~TextEdit()
{
}

void TextEdit::setShowLineNumbers(bool b)
{
	showLineNum_ = b;
	lineNumArea_->setVisible(b);
	updateLineNumberAreaWidth(0);
}


// ---------------------------------------------------------------------------
// TextEdit::cursorRowCol
// returns the row and column position of the cursor
//  - note that the first row and column are (1,1)
// ---------------------------------------------------------------------------

void TextEdit::cursorRowCol(int *row, int *col)
{
    const QTextCursor cursor = textCursor();

    QTextBlock cb, b;
    int column, line = 1;
    cb = cursor.block();
    column = (cursor.position() - cb.position()) + 1;

    // find the line number - is there a better way than this?

    for (b = document()->begin(); b != document()->end(); b = b.next())
    {
        if( b==cb ) break;
        line++;
    }

    *row = line;
    *col = column;
}


// ---------------------------------------------------------------------------
// TextEdit::characterBehindCursor
// returns the character to the left of the text cursor
// ---------------------------------------------------------------------------

QChar TextEdit::characterBehindCursor(QTextCursor *cursor)
{
    QTextCursor docTextCursor = textCursor();
    QTextCursor *theCursor = (cursor == 0) ? &docTextCursor : cursor;
    return document()->characterAt(theCursor->position() - 1);
}

// ---------------------------------------------------------------------------
// TextEdit::numLinesSelected
// returns the number of lines in the current selection
// yes - all this code to do that!
// ---------------------------------------------------------------------------

int TextEdit::numLinesSelected()
{
    QTextCursor cursor = textCursor();   // get the document's cursor
    int selStart = cursor.selectionStart();
    int selEnd   = cursor.selectionEnd();
    QTextBlock bStart = document()->findBlock(selStart);
    QTextBlock bEnd   = document()->findBlock(selEnd);
    int lineStart = bStart.firstLineNumber();
    int lineEnd   = bEnd.firstLineNumber();
    int numLines = (lineEnd - lineStart) + 1;

    return numLines;
}

// ---------------------------------------------------------------------------
// TextEdit::lineNumberAreaWidth
// returns the required width to display the line numbers. This adapts to the
// maximum number of digits we need to display.
// ---------------------------------------------------------------------------

int TextEdit::lineNumberAreaWidth()
{
    if (showLineNumbers())
    {
        int digits = 1;
        int max = qMax(1, blockCount());
        while (max >= 10)
        {
            max /= 10;
            ++digits;
        }

        int space = 3 + fontMetrics().width(QLatin1Char('9')) * digits + rightMargin_;

        return space;
    }
    else
    {
        return 0;
    }
}

// ---------------------------------------------------------------------------
// TextEdit::updateLineNumberAreaWidth
// called when the number of lines in the document changes. The argument is
// the new number of lines (blocks).
// ---------------------------------------------------------------------------

void TextEdit::updateLineNumberAreaWidth(int)
{
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}


// ---------------------------------------------------------------------------
// TextEdit::updateLineNumberArea
// called when the editor is updated. We want to ensure that the line number
// widget stays in sync with it.
// ---------------------------------------------------------------------------

void TextEdit::updateLineNumberArea(const QRect &rect, int dy)
{
    if (dy)
        lineNumArea_->scroll(0, dy);
    else
        lineNumArea_->update(0, rect.y(), lineNumArea_->width(), rect.height());

    if (rect.contains(viewport()->rect()))
        updateLineNumberAreaWidth(0);
}


// ---------------------------------------------------------------------------
// TextEdit::resizeEvent
// called when a resize event is triggered. Reset the size of the line widget.
// ---------------------------------------------------------------------------

void TextEdit::resizeEvent(QResizeEvent *e)
{
    QPlainTextEdit::resizeEvent(e);

    QRect cr = contentsRect();
    lineNumArea_->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}


// ---------------------------------------------------------------------------
// TextEdit::focusInEvent
// called when the widget gains input focus
// ---------------------------------------------------------------------------

void TextEdit::focusInEvent(QFocusEvent *event)
{
    Q_EMIT focusRegained();
    QPlainTextEdit::focusInEvent(event);
}


// ---------------------------------------------------------------------------
// TextEdit::focusOutEvent
// called when the widget loses input focus

// ---------------------------------------------------------------------------

void TextEdit::focusOutEvent(QFocusEvent *event)
{
    Q_EMIT focusLost();
    QPlainTextEdit::focusOutEvent(event);
}


// ---------------------------------------------------------------------------
// TextEdit::lineNumberAreaPaintEvent
// called when the line number widget needs to be repainted. This is where we
// actually draw the numbers on the widget.
// ---------------------------------------------------------------------------

void TextEdit::lineNumberAreaPaintEvent(QPaintEvent *event)
{
    int currentRow, currentCol;
    cursorRowCol (&currentRow, &currentCol);  // get the current line number so we can highlight it


    QPainter painter(lineNumArea_);
    painter.fillRect(event->rect(), QColor(240, 240, 240));  // light grey background

    painter.setPen(QPen(QColor(220,220,220)));
    painter.drawLine(event->rect().topRight(),event->rect().bottomRight());

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = (int) blockBoundingGeometry(block).translated(contentOffset()).top();
    int bottom = top + (int) blockBoundingRect(block).height();
    QFont fontNormal(font());   // the font to use for most line numbers
    QFont fontBold(fontNormal);  // the font to use for the current line number
    fontBold.setBold(true);
    painter.setPen(Qt::blue);

    painter.setFont(fontNormal);

    while (block.isValid() && top <= event->rect().bottom())
    {
        if (block.isVisible() && bottom >= event->rect().top())
        {
            QString number = QString::number(blockNumber + 1);

            if (blockNumber == currentRow-1)  // is this the current line?
            {
                painter.setFont(fontBold);
                painter.fillRect(0, top, lineNumArea_->width()-rightMargin_, fontMetrics().height(), QColor(212, 212, 255));  // highlight the background
            }


            painter.drawText(0, top, lineNumArea_->width()-rightMargin_, fontMetrics().height(),  // draw the line number
                             Qt::AlignRight, number);


            if (blockNumber == currentRow-1)  // is this the current line?
            {
                painter.setFont(fontNormal);  // reset the font to normal
            }
        }

        block = block.next();
        top = bottom;
        bottom = top + (int) blockBoundingRect(block).height();
        ++blockNumber;
    }
}


QString TextEdit::emptyString_ ;  // used as a default argument to findString()

bool TextEdit::findString(const QString &s,QTextDocument::FindFlags flags, bool replace, const QString &r)
{

    lastFindString_ = s;      // store for repeat searches
    lastFindFlags_  = flags;  // store for repeat searches
    bool found = false;

    if (find(s,flags))  // find and select the string - were we successful?
    {
        //statusMessage("", 0);
        found = true;
    }
    else    // did not find the string
    {
        if (1)  // 'wraparound' search - on by default, we can add a user option if it might be useful to turn it off
        {
            QTextCursor original_cursor = textCursor();   // get the document's cursor
            QTextCursor cursor(original_cursor);

            if (flags & QTextDocument::FindBackward)        // move to the start or end of the document to continue the search
                cursor.movePosition(QTextCursor::End);
            else
                cursor.movePosition(QTextCursor::Start);

            setTextCursor(cursor);              // send the cursor back to the document

            if (find(s,flags))                  // search again, from the new position
            {
                //statusMessage("", 0);
                found = true;
            }
            else
            {
                setTextCursor(original_cursor);  // not found - restore the cursor to its original position
            }
        }
    }


    if (found)
    {
        if (replace)
        {
            // perform the 'replace'
            insertPlainText (r);
            
            // highlight the replaced text - the current text cursor will be
            // at the end of the replaced text, so we move it back to the start
            // (anchored so that the text is selected)
            QTextCursor cursor = textCursor();   // get the document's cursor
            cursor.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor, r.length());
            setTextCursor(cursor);               // send the cursor back to the document
        }
        ensureCursorVisible();
    }

    else
    {
        //statusMessage(tr("Searched whole file, string not found"), 5000);
    }

    return found;
}
