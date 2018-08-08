//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "PlainTextEdit.hpp"

#include "GotoLineDialog.hpp"

#include <QtGlobal>
#include <QDebug>
#include <QFile>
#include <QPainter>
#include <QTextBlock>
#include <QWheelEvent>

#include "VConfig.hpp"
#include "UiLog.hpp"

PlainTextEdit::PlainTextEdit(QWidget * parent) :
    QPlainTextEdit(parent),
    showLineNum_(true),
    rightMargin_(2),
    hyperlinkEnabled_(false),
    gotoLineDialog_(nullptr),    
    numAreaBgCol_(232,231,230),
    numAreaFontCol_(102,102,102),
    numAreaSeparatorCol_(210,210,210),
    numAreaCurrentCol_(212,212,255),
    fontProp_(nullptr)
{
    lineNumArea_ = new LineNumberArea(this);

    connect(this,SIGNAL(blockCountChanged(int)),
    		this,SLOT(updateLineNumberAreaWidth(int)));

    connect(this,SIGNAL(updateRequest(QRect,int)),
    		this,SLOT(updateLineNumberArea(QRect,int)));

    connect(this,SIGNAL(cursorPositionChanged()),
    		lineNumArea_,SLOT(update()));


    if(VProperty* p=VConfig::instance()->find("view.textEdit.numAreaBackground"))
        numAreaBgCol_=p->value().value<QColor>();

    if(VProperty* p=VConfig::instance()->find("view.textEdit.numAreaFontColour"))
        numAreaFontCol_=p->value().value<QColor>();

    if(VProperty* p=VConfig::instance()->find("view.textEdit.numAreaSeparator"))
        numAreaSeparatorCol_=p->value().value<QColor>();

    if(VProperty* p=VConfig::instance()->find("view.textEdit.numAreaCurrent"))
        numAreaCurrentCol_=p->value().value<QColor>();

    updateLineNumberAreaWidth(0);

    QFont f("Courier");
    //QFont f("Monospace");
    //f.setStyleHint(QFont::TypeWriter);
    f.setFixedPitch(true);
    f.setPointSize(10);
    //f.setStyleStrategy(QFont::PreferAntialias);
    setFont(f);
}

PlainTextEdit::~PlainTextEdit()
{
    if (gotoLineDialog_)
    	delete gotoLineDialog_;

    if(fontProp_)
    	fontProp_->removeObserver(this);
}

bool PlainTextEdit::setHyperlinkEnabled(bool h)
{
    hyperlinkEnabled_ = h;
    setMouseTracking(h);
    return true;
}

void PlainTextEdit::setShowLineNumbers(bool b)
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

void PlainTextEdit::cursorRowCol(int *row, int *col)
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

QChar PlainTextEdit::characterBehindCursor(QTextCursor *cursor)
{
    QTextCursor docTextCursor = textCursor();
    QTextCursor *theCursor = (cursor == nullptr) ? &docTextCursor : cursor;
    return document()->characterAt(theCursor->position() - 1);
}

// ---------------------------------------------------------------------------
// TextEdit::numLinesSelected
// returns the number of lines in the current selection
// yes - all this code to do that!
// ---------------------------------------------------------------------------

int PlainTextEdit::numLinesSelected()
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

int PlainTextEdit::lineNumberAreaWidth()
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

void PlainTextEdit::updateLineNumberAreaWidth(int)
{
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}


// ---------------------------------------------------------------------------
// TextEdit::updateLineNumberArea
// called when the editor is updated. We want to ensure that the line number
// widget stays in sync with it.
// ---------------------------------------------------------------------------

void PlainTextEdit::updateLineNumberArea(const QRect &rect, int dy)
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

void PlainTextEdit::resizeEvent(QResizeEvent *e)
{
    QPlainTextEdit::resizeEvent(e);

    QRect cr = contentsRect();
    lineNumArea_->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}


// ---------------------------------------------------------------------------
// TextEdit::focusInEvent
// called when the widget gains input focus
// ---------------------------------------------------------------------------

void PlainTextEdit::focusInEvent(QFocusEvent *event)
{
    Q_EMIT focusRegained();
    QPlainTextEdit::focusInEvent(event);
}


// ---------------------------------------------------------------------------
// TextEdit::focusOutEvent
// called when the widget loses input focus

// ---------------------------------------------------------------------------

void PlainTextEdit::focusOutEvent(QFocusEvent *event)
{
    Q_EMIT focusLost();
    QPlainTextEdit::focusOutEvent(event);
}

// ---------------------------------------------------------------------------
// TextEdit::lineNumberAreaPaintEvent
// called when the line number widget needs to be repainted. This is where we
// actually draw the numbers on the widget.
// ---------------------------------------------------------------------------

void PlainTextEdit::lineNumberAreaPaintEvent(QPaintEvent *event)
{
    int currentRow, currentCol;
    cursorRowCol (&currentRow, &currentCol);  // get the current line number so we can highlight it


    QPainter painter(lineNumArea_);
    painter.fillRect(event->rect(), numAreaBgCol_);  // light grey background

    painter.setPen(QPen(numAreaSeparatorCol_));
    painter.drawLine(event->rect().topRight(),event->rect().bottomRight());

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = (int) blockBoundingGeometry(block).translated(contentOffset()).top();
    int bottom = top + (int) blockBoundingRect(block).height();
    QFont fontNormal(font());   // the font to use for most line numbers
    QFont fontBold(fontNormal);  // the font to use for the current line number
    fontBold.setBold(true);
    //painter.setPen(Qt::blue);
    painter.setPen(numAreaFontCol_);

    painter.setFont(fontNormal);

    while (block.isValid() && top <= event->rect().bottom())
    {
        if (block.isVisible() && bottom >= event->rect().top())
        {
            QString number = QString::number(blockNumber + 1);

            if (blockNumber == currentRow-1)  // is this the current line?
            {
                painter.setFont(fontBold);
                painter.fillRect(0, top, lineNumArea_->width()-rightMargin_, fontMetrics().height(), numAreaCurrentCol_);  // highlight the background
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


QString PlainTextEdit::emptyString_ ;  // used as a default argument to findString()

bool PlainTextEdit::findString(const QString &s,QTextDocument::FindFlags flags, bool replace, const QString &r)
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
        if (true)  // 'wraparound' search - on by default, we can add a user option if it might be useful to turn it off
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


// ---------------------------------------------------------------------------
// TextEdit::gotoLine
// triggered when the user asks to bring up the 'go to line' dialog
// ---------------------------------------------------------------------------

void PlainTextEdit::gotoLine()
{
    // create the dialog if it does not already exist

    if (!gotoLineDialog_) 
    {
        gotoLineDialog_ = new GotoLineDialog(this);

        connect(gotoLineDialog_, SIGNAL(gotoLine(int)), this, SLOT(gotoLine(int)));
    }


    // if created, set it up and display it

    if (gotoLineDialog_) 
    {
        gotoLineDialog_->show();
        gotoLineDialog_->raise();
        gotoLineDialog_->activateWindow();
        gotoLineDialog_->setupUIBeforeShow();
    }
}

// ---------------------------------------------------------------------------
// TextEdit::gotoLine
// triggered from the GotoLine dialog when the user wants to go to that line
// ---------------------------------------------------------------------------

void PlainTextEdit::gotoLine(int line)
{
    int bn = 0;
    QTextBlock b;

    if (line <= document()->blockCount())
    {
        for (b = document()->begin(); b != document()->end(); b = b.next())
        {
            if (bn == line-1)
            {
                QTextCursor cursor = textCursor();   // get the document's cursor
                cursor.setPosition (b.position());               // set it to the right position
                cursor.select(QTextCursor::LineUnderCursor);     // select the whole line
                setTextCursor(cursor);               // send the cursor back to the document
                break;
            }
            bn++;
        }
    }
    
    else
    {
        // line number outside range of line numbers
        // TODO: disable the 'ok' button if the number is out of range
    }
}

//---------------------------------------------
// Fontsize management
//---------------------------------------------

void PlainTextEdit::setFontProperty(VProperty* p)
{
	fontProp_=p;
	fontProp_->addObserver(this);
	updateFont();
}

void PlainTextEdit::wheelEvent(QWheelEvent *event)
{
	int fps=font().pointSize();

	if(isReadOnly())
	{
		QPlainTextEdit::wheelEvent(event);
		if(font().pointSize() != fps)
			fontSizeChangedByZoom();
	}
	//For readOnly document the zoom does not work so we
	//need this custom code!
	else
	{
		if(event->modifiers() & Qt::ControlModifier)
		{
			const int delta = event->delta();
	        if (delta < 0)
	        	slotZoomOut();
	        else if (delta > 0)
	            slotZoomIn();
	        return;
		}

		QPlainTextEdit::wheelEvent(event);
	}
}

void PlainTextEdit::slotZoomIn()
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 1, 1)
	zoomIn();
#else
	QFont f=font();
	int fps=f.pointSize();
	f.setPointSize(fps+1);
	setFont(f);
#endif
	fontSizeChangedByZoom();
}

void PlainTextEdit::slotZoomOut()
{
	int oriSize=font().pointSize();

#if QT_VERSION >= QT_VERSION_CHECK(5, 1, 1)
	zoomOut();
#else
	QFont f=font();
	int fps=f.pointSize();
	if(fps > 1)
	{
		f.setPointSize(fps-1);
		setFont(f);
	}
#endif

	if(font().pointSize() != oriSize)
		fontSizeChangedByZoom();
}

void PlainTextEdit::fontSizeChangedByZoom()
{
	if(fontProp_)
		fontProp_->setValue(font());
}

void PlainTextEdit::updateFont()
{
	if(fontProp_)
	{
		QFont f=fontProp_->value().value<QFont>();
		if(font() != f)
			setFont(f);
	}
}

void PlainTextEdit::notifyChange(VProperty* p)
{
	if(fontProp_ ==p)
	{
		setFont(p->value().value<QFont>());
	}
}


void PlainTextEdit::mousePressEvent(QMouseEvent *e)
{
    if (hyperlinkEnabled_)
    {
        // left button only - if pressed, we just store the link that was clicked on
        // - we don't want to do anything until the mouse button has been released and
        // we know it hasd not been moved away from the hyperlinked text

        if (e->button() & Qt::LeftButton)
            currentLink_ = anchorAt(e->pos());
        else
            currentLink_ = QString();
    }

    QPlainTextEdit::mousePressEvent(e);
}

void PlainTextEdit::mouseReleaseEvent(QMouseEvent *e)
{
    if (hyperlinkEnabled_)
    {
        // only activate the hyperlink if the user releases the left mouse button on the
        // same link they were on when they pressed the button

        if ((e->button() & Qt::LeftButton) && !currentLink_.isEmpty())
        {
            if (currentLink_ == anchorAt(e->pos()))
            {
                Q_EMIT hyperlinkActivated(currentLink_);
                UiLog().dbg() << "clicked:" << currentLink_;
            }
        }
    }

    QPlainTextEdit::mouseReleaseEvent(e);
}


void PlainTextEdit::mouseMoveEvent(QMouseEvent *e)
{
    if (hyperlinkEnabled_)
    {
        QString thisAnchor = anchorAt(e->pos());

        if (!thisAnchor.isEmpty())
        {
            viewport()->setCursor(Qt::PointingHandCursor);
        }
        else
        {
            viewport()->unsetCursor();
        }
    }

    QPlainTextEdit::mouseMoveEvent(e);
}

