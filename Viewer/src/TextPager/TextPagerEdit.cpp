// Copyright 2010 Anders Bakken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <QScrollArea>
#include <QScrollBar>
#include <QDesktopWidget>
#include <QVariant>
#include "TextPagerEdit.hpp"
#include "TextPagerEdit_p.hpp"
#include "TextPagerCursor_p.hpp"
#include "TextPagerDocument_p.hpp"
#include "TextPagerSearchHighlighter.hpp"
#include "UserMessage.hpp"
#include "VConfig.hpp"

#include <cmath>

//#define _UI_TEXTPAGER_DEBUG
//#define DEBUG_TEXTPAGER_LASTPAGESIZE
//#define DEBUG_TEXTPAGER

#ifdef DEBUG_TEXTPAGER
bool doLog = false;
QString logFileName;
#endif

/*!
    Constructs an empty TextPagerEdit with parent \a parent.
*/

TextPagerEdit::TextPagerEdit(QWidget *parent) :
   QAbstractScrollArea(parent),
   d(new TextEditPrivate(this)),
   useSearchHighlight_(false),
   lineNumArea_(0),
   fontProp_(NULL)
{
    viewport()->setCursor(Qt::IBeamCursor);

    setProperty("pager","1");

#ifdef DEBUG_TEXTPAGER
    if (logFileName.isEmpty())
        logFileName = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmsszzz.log");
#endif

    connect(verticalScrollBar(), SIGNAL(valueChanged(int)), d, SLOT(onScrollBarValueChanged(int)));
    connect(verticalScrollBar(), SIGNAL(actionTriggered(int)), d, SLOT(onScrollBarActionTriggered(int)));
    connect(verticalScrollBar(), SIGNAL(sliderReleased()), d, SLOT(updateScrollBar()));

    setDocument(new TextPagerDocument(this));
    setViewportMargins(0, 0, 0, 0);
    struct {
        QString text;
        const char *member;
        QKeySequence::StandardKey key;
    } shortcuts[] = {
        { tr("Copy"), SLOT(copy()), QKeySequence::Copy },
        { tr("Select All"), SLOT(selectAll()), QKeySequence::SelectAll },
        { QString(), 0, QKeySequence::UnknownKey } };
    for (int i=0; shortcuts[i].member; ++i) {
        d->actions[i] = new QAction(shortcuts[i].text, this);
        d->actions[i]->setShortcutContext(Qt::WidgetShortcut);
        d->actions[i]->setShortcut(QKeySequence(shortcuts[i].key));
        d->actions[i]->setShortcutContext(Qt::WidgetShortcut);
        connect(d->actions[i], SIGNAL(triggered(bool)), this, shortcuts[i].member);
        addAction(d->actions[i]);
    }
    //connect(this, SIGNAL(undoAvailableChanged(bool)), d->actions[UndoAction], SLOT(setEnabled(bool)));
    //connect(this, SIGNAL(redoAvailableChanged(bool)), d->actions[RedoAction], SLOT(setEnabled(bool)));
    //d->actions[UndoAction]->setEnabled(false);
    //d->actions[RedoAction]->setEnabled(false);
    setContextMenuPolicy(Qt::ActionsContextMenu);
    //setCursorVisible(true); // starts blinking
    connect(this, SIGNAL(selectionChanged()), d, SLOT(onSelectionChanged()));


    setReadOnly(true);

    searchHighlight_=new TextPagerSearchHighlighter(this);
}


/*!
    returns the current cursor position
*/

int TextPagerEdit::cursorPosition() const
{
    return d->textCursor.position();
}

/*!
    ensures that \a cursor is visible with \a linesMargin lines of
    margin on each side (if possible)
*/

void TextPagerEdit::ensureCursorVisible(const TextPagerCursor &cursor, int linesMargin)
{
    linesMargin = qMin(d->visibleLines, linesMargin); // ### could this be a problem down at the bottom of the document?
    Q_ASSERT(cursor.document() == document());
    TextPagerCursor above = cursor;
    for (int i=0; i<linesMargin; ++i) {
        if (!above.movePosition(TextPagerCursor::Up))
            break;
    }
    if (above.position() < d->viewportPosition) {
        d->updateViewportPosition(above.position(), TextPagerLayout::Backward);
        return;
    }

    TextPagerCursor below = cursor;
    for (int i=0; i<linesMargin; ++i) {
        if (!below.movePosition(TextPagerCursor::Down))
            break;
    }

    if (below.position() > d->lastVisibleCharacter) {
        for (int i=0; i<d->visibleLines; ++i) {
            below.movePosition(TextPagerCursor::Up);
            d->updateViewportPosition(below.position(), TextPagerLayout::Forward);
        }
    }
}

/*!
    returns the QAction * for \a type.
*/

QAction *TextPagerEdit::action(ActionType type) const
{
    return d->actions[type];
}

/*!
    Called when the textEdit is deleted
*/

TextPagerEdit::~TextPagerEdit()
{
    if (d->document) {
        Q_FOREACH(SyntaxHighlighter *syntaxHighlighter, d->syntaxHighlighters) {
            if (syntaxHighlighter->parent() == this)
                disconnect(syntaxHighlighter, 0, d, 0);
            syntaxHighlighter->d->textEdit = 0;
            syntaxHighlighter->d->textLayout = 0;
        }
        disconnect(d->document, 0, this, 0);
        disconnect(d->document, 0, d, 0);
        disconnect(d->document->d, 0, this, 0);
        disconnect(d->document->d, 0, d, 0);

        if (d->document->parent() == this) {
            delete d->document;
            d->document = 0;
        } else {
            d->document->d->textEditDestroyed(this);
        }
        // to make sure we don't do anything drastic on shutdown
    }
    delete d;

    if(fontProp_)
        fontProp_->removeObserver(this);
}

/*!
    returns the text edit's document. document() always returns a
    valid TextPagerDocument *
    \sa setDocument
*/


TextPagerDocument *TextPagerEdit::document() const
{
    return d->document;
}

/*!
    sets the text edit's document to \a doc. The previous document
    will be deleted if it's parented to the text edit.

    If \a doc is 0 a default TextPagerDocument will be set.
    \sa document
*/

void TextPagerEdit::setDocument(TextPagerDocument *doc)
{
    if (doc == d->document)
        return;

    if (d->document) {
        disconnect(d->document, 0, this, 0);
        disconnect(d->document, 0, d, 0);
        disconnect(d->document->d, 0, this, 0);
        disconnect(d->document->d, 0, d, 0);
        if (d->document->parent() == this)
            delete d->document;
    }
    if (!doc)
        doc = new TextPagerDocument(this);

    d->sections.clear();
    d->buffer.clear();
    d->sectionsDirty = true;
    d->document = doc;
    d->sectionPressed = 0;
    d->layoutDirty = true;
    qDeleteAll(d->unusedTextLayouts);
    d->unusedTextLayouts.clear();
    qDeleteAll(d->textLayouts);
    d->textLayouts.clear();
    viewport()->setCursor(Qt::IBeamCursor);
    viewport()->setMouseTracking(true);
    d->sectionCount = 0;

    d->textCursor = TextPagerCursor(doc);
    d->textCursor.textEdit = this;

    /*connect(d->document->d, SIGNAL(undoRedoCommandInserted(DocumentCommand *)),
            d, SLOT(onDocumentCommandInserted(DocumentCommand *)));
    connect(d->document, SIGNAL(sectionAdded(TextPagerSection *)),
            d, SLOT(onTextSectionAdded(TextPagerSection *)));
    connect(d->document, SIGNAL(sectionRemoved(TextPagerSection *)),
            d, SLOT(onTextSectionRemoved(TextPagerSection *)));
    connect(d->document->d, SIGNAL(undoRedoCommandRemoved(DocumentCommand *)),
            d, SLOT(onDocumentCommandRemoved(DocumentCommand *)));
    connect(d->document->d, SIGNAL(undoRedoCommandTriggered(DocumentCommand *, bool)),
            d, SLOT(onDocumentCommandTriggered(DocumentCommand *, bool)));*/
   
    connect(d->document, SIGNAL(charactersAdded(int, int)),
            d, SLOT(onCharactersAddedOrRemoved(int, int)));
    connect(d->document, SIGNAL(charactersRemoved(int, int)),
            d, SLOT(onCharactersAddedOrRemoved(int, int)));
    
    /*connect(d->document, SIGNAL(textChanged()), this, SIGNAL(textChanged()));
    connect(d->document, SIGNAL(undoAvailableChanged(bool)),
            this, SIGNAL(undoAvailableChanged(bool)));
    connect(d->document, SIGNAL(redoAvailableChanged(bool)),
            this, SIGNAL(redoAvailableChanged(bool)));*/

    connect(d->document, SIGNAL(documentSizeChanged(int)), d, SLOT(onDocumentSizeChanged(int)));
    connect(d->document, SIGNAL(destroyed(QObject*)), d, SLOT(onDocumentDestroyed()));
    connect(d->document->d, SIGNAL(sectionFormatChanged(TextPagerSection *)),
            d, SLOT(onTextSectionFormatChanged(TextPagerSection *)));
    connect(d->document->d, SIGNAL(sectionCursorChanged(TextPagerSection *)),
            d, SLOT(onTextSectionCursorChanged(TextPagerSection *)));

    d->onDocumentSizeChanged(d->document->documentSize());

    viewport()->update();
}

/*!
    returns the textCursor's width (in pixels)
    \sa setCursorWidth
*/

int TextPagerEdit::cursorWidth() const
{
    return d->cursorWidth;
}

/*!
    sets the cursorWidth of the text edit to \a cw pixels.
    \a cw must be a valid integer larger than 0.

    \sa setCursorVisible
*/

void TextPagerEdit::setCursorWidth(int cw)
{
    Q_ASSERT(d->cursorWidth > 0);
    d->cursorWidth = cw;
    viewport()->update();
}

/*!
    Loads the contenst of \a dev into the text edit's document.
    Equivalent to calling document()->load(\a dev, \a mode);

    \sa setCursorVisible
*/

#if 0
bool TextPagerEdit::load(QIODevice *dev, TextPagerDocument::DeviceMode mode, QTextCodec *codec)
{
#ifdef DEBUG_TEXTPAGER
    if (doLog) {
        QFile f(logFileName);
        f.open(QIODevice::WriteOnly);
        QDataStream ds(&f);
        if (QFile *ff = qobject_cast<QFile*>(dev)) {
            ds << ff->fileName();
        } else {
            ds << QString::fromLatin1(dev->metaObject()->className());
        }
    }
#endif
    //we have to check to font here because the initial setting in setFontProperty doe not have any effect
    updateFont();
    return d->document->load(dev, mode, codec);
}
#endif

bool TextPagerEdit::load(const QString &file, TextPagerDocument::DeviceMode mode, QTextCodec *codec)
{
#ifdef DEBUG_TEXTPAGER
    if (doLog) {
        QFile f(logFileName);
        f.open(QIODevice::WriteOnly);
        QDataStream ds(&f);
        ds << file;
    }
#endif

    UserMessage::message(UserMessage::DBG,false,"TextPagerEdit::load fileName" + file.toStdString());
    //we have to check to font here because the initial setting in setFontProperty doe not have any effect
    updateFont();
    bool ret=d->document->load(file, mode, codec);
    UserMessage::message(UserMessage::DBG,false,"  cursor: " + QString::number(textCursor().position()).toStdString());
    return ret;
}

void TextPagerEdit::setText(const QString &txt)
{
    //we have to check to font here because the initial setting in setFontProperty doe not have any effect
    updateFont();
    d->document->setText(txt);
}

enum SelectionAddStatus {
    Invalid,
    Before,
    After,
    Success
};





static inline SelectionAddStatus addSelection(int layoutStart, int layoutLength,
                                              const TextPagerCursor &cursor, QTextLayout::FormatRange *format)
{
    Q_ASSERT(format);
    if (!cursor.hasSelection())
        return Invalid;
    if (cursor.selectionEnd() < layoutStart)
        return Before;
    if (cursor.selectionStart() > layoutStart + layoutLength)
        return After;

    format->start = qMax(0, cursor.selectionStart() - layoutStart);
    format->length = qMin(layoutLength - format->start,
                          cursor.selectionEnd() - layoutStart - format->start);
    return Success;
}

void TextPagerEdit::paintEvent(QPaintEvent *e)
{
    d->updateScrollBarPosition();
    d->relayout();
    if (d->updateScrollBarPageStepPending) {
        d->updateScrollBarPageStepPending = false;
        d->updateScrollBarPageStep();
    }

    QPainter p(viewport());

    const QRect er = e->rect();
    p.translate(-horizontalScrollBar()->value(), 0);
    p.setFont(font());
    QVector<QTextLayout::FormatRange> selections;
    selections.reserve(d->extraSelections.size() + 1);
    int textLayoutOffset = d->viewportPosition;

    const QTextLayout *cursorLayout = d->cursorVisible ? d->layoutForPosition(d->textCursor.position()) : 0;
    int extraSelectionIndex = 0;
    QTextLayout::FormatRange selectionRange;
    selectionRange.start = -1;
    Q_FOREACH(QTextLayout *l, d->textLayouts) {
        const int textSize = l->text().size();
        const QRect r = l->boundingRect().toRect();
        if (r.intersects(er)) {
            const QBrush background = d->blockFormats.value(l).background();
            if (background.style() != Qt::NoBrush) {
                p.fillRect(r, background);
            }
            if (::addSelection(textLayoutOffset, textSize, d->textCursor, &selectionRange) == Success) {
                selectionRange.format.setBackground(palette().highlight());
                selectionRange.format.setForeground(palette().highlightedText());
            }
            int lowestIncompleteSelection = -1;
            while (extraSelectionIndex < d->extraSelections.size()) {
                QTextLayout::FormatRange range;
                const SelectionAddStatus s = ::addSelection(textLayoutOffset, textSize,
                                                            d->extraSelections.at(extraSelectionIndex).
                                                            cursor, &range);
                if (s == Success) {
                    range.format = d->extraSelections.at(extraSelectionIndex).format;
                    selections.append(range);

                    const TextPagerCursor &cursor = d->extraSelections.at(extraSelectionIndex).cursor;
                    int lastPos = cursor.position() + cursor.selectionSize();
                    if (lastPos > textLayoutOffset+textSize && lowestIncompleteSelection < 0) {
                        lowestIncompleteSelection = extraSelectionIndex;
                    }
                } else if (s == After) {
                    break;
                }
                ++extraSelectionIndex;
            }
            if (lowestIncompleteSelection > -1) {
                extraSelectionIndex = lowestIncompleteSelection;
            }

            // is this the current line?
            /*if(cursorLayout == l)
            {
                p.fillRect(r, QColor(216,228,239));  // highlight the current line
            }*/


            if (selectionRange.start != -1) {
                // The last range in the vector has priority, that
                // should probably be the real selection
                selections.append(selectionRange);
                selectionRange.start = -1;
            }

            l->draw(&p, QPoint(0, 0), selections);
            if (!selections.isEmpty())
                selections.clear();
            if (cursorLayout == l) {
                cursorLayout->drawCursor(&p, QPoint(0, 0), d->textCursor.position() - textLayoutOffset,
                                         d->cursorWidth);
            }
        } else if (r.top() > er.bottom()) {
            break;
        }
        textLayoutOffset += l->text().size() + 1;
    }


    //paintLineNumberArea(e);
}

void TextPagerEdit::scrollContentsBy(int dx, int dy)
{
    Q_UNUSED(dx);
    Q_UNUSED(dy);
//    viewport()->update();
    viewport()->scroll(dx, dy); // seems to jitter more
}

int TextPagerEdit::viewportPosition() const
{
    return d->viewportPosition;
}

void TextPagerEdit::mousePressEvent(QMouseEvent *e)
{
    d->inMouseEvent = true;
    if (e->button() == Qt::LeftButton) {
#ifdef DEBUG_TEXTPAGER
        if (doLog) {
            QFile file(logFileName);
            file.open(QIODevice::Append);
            QDataStream ds(&file);
            ds << int(e->type()) << e->pos() << e->button() << e->buttons() << e->modifiers();
        }
#endif
        if (d->tripleClickTimer.isActive()) {
            d->tripleClickTimer.stop();
            d->textCursor.movePosition(TextPagerCursor::StartOfBlock);
            d->textCursor.movePosition(TextPagerCursor::EndOfBlock, TextPagerCursor::KeepAnchor);
        } else {
            const bool shift = e->modifiers() & Qt::ShiftModifier;
            if (!shift) {
                clearSelection();
            }
            int pos = textPositionAt(e->pos());
            if (pos == -1)
                pos = d->document->documentSize() - 1;
            d->sectionPressed = d->document->d->sectionAt(pos, this);
            setCursorPosition(pos, shift ? TextPagerCursor::KeepAnchor : TextPagerCursor::MoveAnchor);
        }

        e->accept();

        //The cursor changed so wee need to update the selected line number
        lineNumArea_->update();
    }
    else {
        QAbstractScrollArea::mousePressEvent(e);
    }
}

void TextPagerEdit::mouseDoubleClickEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton) {
#ifdef DEBUG_TEXTPAGER
        if (doLog) {
            QFile file(logFileName);
            file.open(QIODevice::Append);
            QDataStream ds(&file);
            ds << int(e->type()) << e->pos() << e->button() << e->buttons() << e->modifiers();
        }
#endif
        const int pos = textPositionAt(e->pos());
        if (pos == d->textCursor.position()) {
            d->tripleClickTimer.start(qApp->doubleClickInterval(), d);
            if (d->document->isWordCharacter(d->textCursor.cursorCharacter(),
                                             d->textCursor.position())) {
                // ### this is not quite right
                d->textCursor.movePosition(TextPagerCursor::StartOfWord);
                d->textCursor.movePosition(TextPagerCursor::EndOfWord, TextPagerCursor::KeepAnchor);
                return;
            }
        }
        mousePressEvent(e);
    }
}

void TextPagerEdit::mouseMoveEvent(QMouseEvent *e)
{
    if (e->buttons() == Qt::NoButton) {
        d->updateCursorPosition(e->pos());
    } else if (e->buttons() == Qt::LeftButton && d->document->documentSize()) {
#ifdef DEBUG_TEXTPAGER
        if (doLog) {
            QFile file(logFileName);
            file.open(QIODevice::Append);
            QDataStream ds(&file);
            ds << int(e->type()) << e->pos() << e->button() << e->buttons() << e->modifiers();
        }
#endif
        const QRect r = viewport()->rect();
        d->lastMouseMove = e->pos();
        e->accept();
        if (e->y() < r.top()) {
            if (d->atBeginning()) {
                d->updateHorizontalPosition();
                d->autoScrollTimer.stop();
                return;
            }
        } else if (e->y() > r.bottom()) {
            if (d->atEnd()) {
                d->updateHorizontalPosition();
                d->autoScrollTimer.stop();
                return;
            }
        } else {
            int pos = textPositionAt(QPoint(qBound(0, d->lastMouseMove.x(), r.right()), d->lastMouseMove.y()));
            if (pos == -1)
                pos = d->document->documentSize();
            d->autoScrollTimer.stop();
            setCursorPosition(pos, TextPagerCursor::KeepAnchor);

            //The cursor changed so wee need to update the selected line number
            lineNumArea_->update();

            return;
        }
        const int distance = qMax(r.top() - d->lastMouseMove.y(), d->lastMouseMove.y() - r.bottom());
        Q_ASSERT(distance != 0);
        enum { MinimumTimeOut = 3 };
        int timeout = qMax<int>(MinimumTimeOut, 100 - distance);
        enum { Margin = 3 };
        if (qApp->desktop()->screenGeometry(this).bottom() - mapToGlobal(d->lastMouseMove).y() <= Margin) {
            timeout = MinimumTimeOut;
        }
        d->autoScrollLines = 1 + ((100 - timeout) / 30);
        if (d->autoScrollTimer.isActive()) {
            d->pendingTimeOut = timeout;
        } else {
            d->pendingTimeOut = -1;
            d->autoScrollTimer.start(timeout, d);
        }
    } else {
        QAbstractScrollArea::mouseMoveEvent(e);
    }
}

void TextPagerEdit::mouseReleaseEvent(QMouseEvent *e)
{
    d->inMouseEvent = false;
    if (e->button() == Qt::LeftButton) {
#ifdef DEBUG_TEXTPAGER
        if (doLog) {
            QFile file(logFileName);
            file.open(QIODevice::Append);
            QDataStream ds(&file);
            ds << int(e->type()) << e->pos() << e->button() << e->buttons() << e->modifiers();
        }
#endif
        d->autoScrollTimer.stop();
        d->pendingTimeOut = -1;
        e->accept();
        if (d->sectionPressed && sectionAt(e->pos()) == d->sectionPressed) {
            Q_EMIT sectionClicked(d->sectionPressed, e->pos());
        }
        d->sectionPressed = 0;
    } else {
        QAbstractScrollArea::mouseReleaseEvent(e);
    }
}

void TextPagerEdit::resizeEvent(QResizeEvent *e)
{
#ifdef DEBUG_TEXTPAGER
    if (doLog) {
        QFile file(logFileName);
        file.open(QIODevice::Append);
        QDataStream ds(&file);
        ds << int(e->type()) << e->size();
    }
#endif
    QAbstractScrollArea::resizeEvent(e);
    if(e->oldSize().height() != e->size().height())
        d->adjustVerticalScrollBar();
    d->updateScrollBarPageStepPending = true;
    d->layoutDirty = true;
}

int TextPagerEdit::textPositionAt(const QPoint &pos) const
{
    if (!viewport()->rect().contains(pos))
        return -1;

    QPoint realPos=pos;
    realPos+=QPoint(horizontalScrollBar()->value(),0);

    //Adjust the horizontal position
  /*  if(pos.x()  > horizontalScrollBar()->value()+viewport()->rect().width())
    {
        int hval=realCrect.left()-r.width()/2;
        horizontalScrollBar()->setValue((hval < horizontalScrollBar()->maximum())?
                                            hval:
                                            horizontalScrollBar()->maximum());
    }
    else if(realCrect.right() < horizontalScrollBar()->value())
    {
        int hval=realCrect.left()-r.width()/2;
        horizontalScrollBar()->setValue((hval > horizontalScrollBar()->minimum())?
                                            hval:
                                            horizontalScrollBar()->minimum());
    }
}*/






    return d->textPositionAt(realPos);
}

bool TextPagerEdit::readOnly() const
{
    return d->readOnly;
}

void TextPagerEdit::setReadOnly(bool rr)
{
    d->readOnly = rr;

    setCursorVisible(!rr);

  /*  //d->actions[PasteAction]->setEnabled(!rr);
    //d->actions[CutAction]->setEnabled(!rr);
    //d->actions[PasteAction]->setVisible(!rr);
    //d->actions[CutAction]->setVisible(!rr);

    const bool redoWasAvailable = isRedoAvailable();
    const bool undoWasAvailable = isUndoAvailable();

   // d->actions[UndoAction]->setEnabled(!rr);
  //  d->actions[RedoAction]->setEnabled(!rr);
   // d->actions[UndoAction]->setVisible(!rr);
   // d->actions[RedoAction]->setVisible(!rr);


    if (undoWasAvailable != isUndoAvailable())
        Q_EMIT undoAvailableChanged(!undoWasAvailable);
    if (redoWasAvailable != isRedoAvailable())
        Q_EMIT redoAvailableChanged(!redoWasAvailable);
        */
}

bool TextPagerEdit::lineBreaking() const
{
    return d->lineBreaking;
}

void TextPagerEdit::setLineBreaking(bool lineBreaking)
{
    if (d->lineBreaking != lineBreaking) {
        d->lineBreaking = lineBreaking;
        d->layoutDirty = true;
        viewport()->update();
    }
}


int TextPagerEdit::maximumSizeCopy() const
{
    return d->maximumSizeCopy;
}

void TextPagerEdit::setMaximumSizeCopy(int max)
{
    d->maximumSizeCopy = qMax(0, max);
    d->updateCopyAndCutEnabled();
}

QRect TextPagerEdit::cursorBlockRect(const TextPagerCursor &textCursor) const
{
    if (const QTextLayout *l = d->layoutForPosition(textCursor.position())) {
        return l->boundingRect().toRect();
    }
    return QRect();
}

QRect TextPagerEdit::cursorRect(const TextPagerCursor &textCursor) const
{
    int offset = -1;
    if (d->layoutForPosition(textCursor.position(), &offset)) {
        ASSUME(offset != -1);
        QTextLine line = d->lineForPosition(textCursor.position());

        //if line is empty line.cursorToX() crashes. A qt bug?
        if(line.isValid()) {
        	qreal x = line.cursorToX(offset);
        	return QRect(x, line.y(), d->cursorWidth, line.height());
        }
    }
    return QRect();
}

int TextPagerEdit::lineNumber(int position) const
{
    return d->document->lineNumber(position);
}

int TextPagerEdit::columnNumber(int position) const
{
    TextPagerCursor cursor(this, position);
    return cursor.isNull() ? -1 : cursor.columnNumber();
}

int TextPagerEdit::lineNumber(const TextPagerCursor &cursor) const
{
    return cursor.document() == d->document
        ? d->document->lineNumber(cursor.position()) : -1;
}

int TextPagerEdit::columnNumber(const TextPagerCursor &cursor) const
{
    return cursor.document() == d->document
        ? cursor.columnNumber() : -1;
}

void TextPagerEdit::wheelEvent(QWheelEvent *e)
{
    if (e->orientation() == Qt::Vertical) {
        d->scrollLines(3 * (e->delta() > 0 ? -1 : 1));
    } else {
        QAbstractScrollArea::wheelEvent(e);
    }

    if(e->modifiers() & Qt::ControlModifier)
    {
    	const int delta = e->delta();
        if (delta < 0)
    	   zoomOut();
    	else if (delta > 0)
    	   zoomIn();
    	return;
     }
}

void TextPagerEdit::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::FontChange) {
        d->font = font();
        Q_FOREACH(QTextLayout *l, d->textLayouts) {
            l->setFont(d->font);
        }
        Q_FOREACH(QTextLayout *l, d->unusedTextLayouts) {
            l->setFont(d->font);
        }
      
        d->adjustVerticalScrollBar();
        
        lineNumArea_->updateWidth();

        d->layoutDirty = true;
        viewport()->update();
    }
}

void TextPagerEdit::keyPressEvent(QKeyEvent *e)
{
#ifdef DEBUG_TEXTPAGER
    if (doLog) {
        QFile file(logFileName);
        file.open(QIODevice::Append);
        QDataStream ds(&file);
        ds << int(e->type()) << e->key() << int(e->modifiers()) << e->text() << e->isAutoRepeat() << e->count();
    }
#endif

    Q_ASSERT(d->textCursor.textEdit == this);
    if (d->readOnly) {
        d->cursorMoveKeyEventReadOnly(e);
        return;
    }
}

void TextPagerEdit::keyReleaseEvent(QKeyEvent *e)
{
#ifdef DEBUG_TEXTPAGER
    if (doLog) { // ### does it make any sense to replay these? Probably not
        QFile file(logFileName);
        file.open(QIODevice::Append);
        QDataStream ds(&file);
        ds << int(e->type()) << e->key() << int(e->modifiers()) << e->text() << e->isAutoRepeat() << e->count();
    }
#endif
    QAbstractScrollArea::keyReleaseEvent(e);
}

void TextPagerEdit::setCursorPosition(int pos, TextPagerCursor::MoveMode mode)
{
    d->textCursor.setPosition(pos, mode);
}

bool TextPagerEdit::moveCursorPosition(TextPagerCursor::MoveOperation op, TextPagerCursor::MoveMode mode, int n)
{
    return d->textCursor.movePosition(op, mode, n);
}

void TextPagerEdit::copy(QClipboard::Mode mode)
{
    if (d->textCursor.selectionSize() <= d->maximumSizeCopy) {
        QApplication::clipboard()->setText(selectedText(), mode);
    }
}

bool TextPagerEdit::cursorVisible() const
{
    return d->cursorBlinkTimer.isActive();
}

void TextPagerEdit::setCursorVisible(bool cc)
{
    if (cc == d->cursorBlinkTimer.isActive())
        return;

    d->cursorVisible = cc;
    if (cc) {
        d->cursorBlinkTimer.start(QApplication::cursorFlashTime(), d);
    } else {
        d->cursorBlinkTimer.stop();
    }
    const QRect r = cursorRect(d->textCursor) & viewport()->rect();
    if (!r.isNull()) {
        viewport()->update(r);
    }
}

void TextPagerEdit::clearSelection()
{
    if (!d->textCursor.hasSelection())
        return;

    d->textCursor.clearSelection();
}

QString TextPagerEdit::selectedText() const
{
    return d->textCursor.selectedText();
}

bool TextPagerEdit::hasSelection() const
{
    return d->textCursor.hasSelection();
}

void TextPagerEdit::selectAll()
{
    TextPagerCursor cursor(d->document);
    Q_ASSERT(cursor.position() == 0);
    cursor.movePosition(TextPagerCursor::End, TextPagerCursor::KeepAnchor);
    setTextCursor(cursor);
    Q_EMIT selectionChanged();
}

QString TextPagerEdit::read(int pos, int size) const
{
    return d->document->read(pos, size);
}

QChar TextPagerEdit::readCharacter(int index) const
{
    return d->document->readCharacter(index);
}

void TextEditPrivate::onDocumentSizeChanged(int size)
{
    adjustVerticalScrollBar();
    
    /*int s=findLastPageSize();
    
    qDebug() << "lat page position" << s;
    
    if(s != -1)
       textEdit->verticalScrollBar()->setRange(0, s); 
    else
       textEdit->verticalScrollBar()->setRange(0, qMax(0, size)); 
    
    
    //textEdit->verticalScrollBar()->setRange(0, qMax(0, size));
//    qDebug() << findLastPageSize();
    maxViewportPosition = textEdit->verticalScrollBar()->maximum();
    updateScrollBarPageStepPending = true;*/
}


void TextEditPrivate::adjustVerticalScrollBar()
{
    int s=findLastPageSize();
    
    int size=(document != 0)?(document->documentSize()):0;
  
#ifdef DEBUG_TEXTPAGER_LASTPAGESIZE    
    qDebug() << "last page position" << s;
#endif    
    if(s == 0) {
        textEdit->verticalScrollBar()->hide();
        textEdit->verticalScrollBar()->setEnabled(false);
    } else {
        textEdit->verticalScrollBar()->setEnabled(true);
        textEdit->verticalScrollBar()->show();
 
        if(s != -1)
            textEdit->verticalScrollBar()->setRange(0, s); 
        else
            textEdit->verticalScrollBar()->setRange(0, qMax(0, size)); 
    }
    
    //textEdit->verticalScrollBar()->setRange(0, qMax(0, size));
//    qDebug() << findLastPageSize();
    maxViewportPosition = textEdit->verticalScrollBar()->maximum();
    updateScrollBarPageStepPending = true;
}

void TextEditPrivate::updateCopyAndCutEnabled()
{
    const bool wasEnabled = actions[TextPagerEdit::CopyAction]->isEnabled();
    const bool enable = qAbs(textCursor.position() - textCursor.anchor()) <= maximumSizeCopy;
    actions[TextPagerEdit::CopyAction]->setEnabled(enable);
    //actions[TextPagerEdit::CutAction]->setEnabled(enable);
    if (wasEnabled != enable) {
        Q_EMIT textEdit->copyAvailable(enable);
    }
}

void TextPagerEdit::takeSyntaxHighlighter(SyntaxHighlighter *highlighter)
{
    Q_ASSERT(highlighter);
    const bool found = d->syntaxHighlighters.removeOne(highlighter);
    Q_ASSERT(found);
    Q_UNUSED(found);
    highlighter->d->textEdit = 0;
    highlighter->d->textLayout = 0;
    disconnect(highlighter, 0, d, 0);
}

void TextPagerEdit::removeSyntaxHighlighter(SyntaxHighlighter *highlighter)
{
    takeSyntaxHighlighter(highlighter);
    delete highlighter;
}

void TextPagerEdit::addSyntaxHighlighter(SyntaxHighlighter *highlighter)
{
    Q_ASSERT(highlighter);
    if (highlighter->textEdit() != this) {
        if (highlighter->textEdit()) {
            qWarning("A SyntaxHighlighter can only be added to 1 TextPagerEdit. If this is a "
                     "use case you care about (having a syntaxHighlighter added to multiple "
                     "text edits) I could fix it. Anders");
            return;
        }
        d->syntaxHighlighters.append(highlighter);
        connect(highlighter, SIGNAL(destroyed(QObject*)), d, SLOT(onSyntaxHighlighterDestroyed(QObject*)));
        highlighter->d->textEdit = this;
        highlighter->d->textLayout = d;
        d->layoutDirty = true;
        viewport()->update();
    }
}

void TextPagerEdit::clearSyntaxHighlighters()
{
    Q_FOREACH(SyntaxHighlighter *highlighter, d->syntaxHighlighters) {
        if (highlighter->parent() == this) {
            removeSyntaxHighlighter(highlighter);
        } else {
            takeSyntaxHighlighter(highlighter);
        }
    }
    Q_ASSERT(d->syntaxHighlighters.isEmpty());
}


void TextEditPrivate::onSyntaxHighlighterDestroyed(QObject *o)
{
    const bool found = syntaxHighlighters.removeOne(static_cast<SyntaxHighlighter*>(o));
    Q_ASSERT(found);
    Q_UNUSED(found);
    layoutDirty = true;
    textEdit->viewport()->update();
}

QList<SyntaxHighlighter*> TextPagerEdit::syntaxHighlighters() const
{
    return d->syntaxHighlighters;
}

static inline bool compareExtraSelection(const TextPagerEdit::ExtraSelection &left, const TextPagerEdit::ExtraSelection &right)
{
    return left.cursor < right.cursor;
}

void TextPagerEdit::setExtraSelections(const QList<ExtraSelection> &selections)
{
    d->extraSelections = selections;
    qSort(d->extraSelections.begin(), d->extraSelections.end(), compareExtraSelection);
    d->layoutDirty = true;
    viewport()->update();
}

QList<TextPagerEdit::ExtraSelection> TextPagerEdit::extraSelections() const
{
    return d->extraSelections;
}

void TextEditPrivate::onTextSectionRemoved(TextPagerSection *section)
{
    Q_ASSERT(section);
    if (!dirtyForSection(section))
        return;

    sectionsDirty = true;
    if (section == sectionPressed) {
        sectionPressed = 0;
    }
    if (section->hasCursor()) {
        updateCursorPosition(lastHoverPos);
    }
}

void TextEditPrivate::onTextSectionAdded(TextPagerSection *section)
{
    dirtyForSection(section);
    updateCursorPosition(lastHoverPos);
    sectionsDirty = true;
}

void TextEditPrivate::onScrollBarValueChanged(int value)
{
    if (blockScrollBarUpdate || value == requestedScrollBarPosition || value == viewportPosition)
        return;
    requestedScrollBarPosition = value;
    layoutDirty = true;
    textEdit->viewport()->update();

    Q_EMIT scrollBarChanged();
}

void TextEditPrivate::onScrollBarActionTriggered(int action)
{
    switch (action) {
    case QAbstractSlider::SliderSingleStepAdd:
        scrollLines(1); requestedScrollBarPosition = -1; break;
    case QAbstractSlider::SliderSingleStepSub:
        scrollLines(-1); requestedScrollBarPosition = -1; break;
    default: break;
    }

    Q_EMIT scrollBarChanged();
}

void TextEditPrivate::updateScrollBar()
{
    Q_ASSERT(!textEdit->verticalScrollBar()->isSliderDown());
    if (pendingScrollBarUpdate) {
        const bool old = blockScrollBarUpdate;
        blockScrollBarUpdate = true;
        textEdit->verticalScrollBar()->setValue(viewportPosition);
        blockScrollBarUpdate = old;
        pendingScrollBarUpdate  = false;

        Q_EMIT scrollBarChanged();
    }
}

void TextEditPrivate::onCharactersAddedOrRemoved(int from, int count)
{
    Q_ASSERT(count >= 0);
    Q_UNUSED(count);

    textCursor.clearSelection();

    if(textCursor.position() > document->documentSize())
        textCursor.setPosition(0);

    UserMessage::message(UserMessage::DBG,false,
              "TextEditPrivate::onCharactersAddedOrRemoved --> textCursor: " + QString::number(textCursor.position()).toStdString());

    /*if (from > qMin(bufferPosition + buffer.size(), layoutEnd)) {
        return;
    }*/
    buffer.clear(); // isn't it better to just add them here?
    layoutDirty = true;
    textEdit->viewport()->update();
}

void TextPagerEdit::ensureCursorVisible()
{
    if (d->textCursor.position() < d->viewportPosition) {
        d->updateViewportPosition(qMax(0, d->textCursor.position() - 1), TextPagerLayout::Backward);
    } else if (d->textCursor.position() > d->layoutEnd) {
        d->updateViewportPosition(d->textCursor.position(), TextPagerLayout::Backward);
        viewport()->update();
    } else {
        const QRect r = viewport()->rect();
        QRect realCrect = cursorRect(d->textCursor);
        QRect crect = realCrect;
        crect.setLeft(r.left());
        crect.setRight(r.right());
        // ### what if the cursor is out of bounds horizontally?

        //qDebug() << "ensureCursorVisible" << r << crect << cursorRect(d->textCursor) << horizontalScrollBar()->value() << horizontalScrollBar()->maximum() <<  d->widest;
        if (!r.contains(crect)) {
            if (r.intersects(crect)) {
                int scroll;
                if (d->autoScrollLines != 0) {
                    scroll = d->autoScrollLines;
                } else {
                    scroll = (crect.top() < r.top() ? -1 : 1);
                }
                d->scrollLines(scroll);
            } else {
                d->updateViewportPosition(d->textCursor.position(), TextPagerLayout::Backward);
            }
            viewport()->update();
        }

        //Adjust the horizontal position
        if(realCrect.left() > horizontalScrollBar()->value()+r.width())
        {
            int hval=realCrect.left()-r.width()/2;
            horizontalScrollBar()->setValue((hval < horizontalScrollBar()->maximum())?
                                                hval:
                                                horizontalScrollBar()->maximum());
        }
        else if(realCrect.right() < horizontalScrollBar()->value())
        {
            int hval=realCrect.left()-r.width()/2;
            horizontalScrollBar()->setValue((hval > horizontalScrollBar()->minimum())?
                                                hval:
                                                horizontalScrollBar()->minimum());
        }
    }
}

//textEdit->horizontalScrollBar()->setPageStep(s.width());
//textEdit->horizontalScrollBar()->setMaximum(qMax(0, widest - s.width()));


TextPagerCursor &TextPagerEdit::textCursor()
{
    return d->textCursor;
}

const TextPagerCursor &TextPagerEdit::textCursor() const
{
    return d->textCursor;
}

void TextPagerEdit::setTextCursor(const TextPagerCursor &textCursor)
{
    const bool doEmit = (d->textCursor != textCursor);
    d->textCursor = textCursor;
    d->textCursor.textEdit = this;
    if (doEmit) {
        ensureCursorVisible();
        viewport()->update();
        Q_EMIT cursorPositionChanged(textCursor.position());
    }
}

TextPagerCursor TextPagerEdit::cursorForPosition(const QPoint &pos) const
{
    const int idx = textPositionAt(pos);
    if (idx == -1)
        return TextPagerCursor();
    return TextPagerCursor(this, idx);
}

TextPagerSection *TextPagerEdit::sectionAt(const QPoint &pos) const
{
    Q_ASSERT(d->document);
    int textPos = textPositionAt(pos);
    if (textPos == -1)
        textPos = d->document->d->documentSize - 1;
    return d->document->d->sectionAt(textPos, this);
}

QList<TextPagerSection*> TextPagerEdit::sections(int from, int size, TextPagerSection::TextSectionOptions opt) const
{
    Q_ASSERT(d->document);
    QList<TextPagerSection*> sections = d->document->d->getSections(from, size, opt, this);
    return sections;
}

TextPagerSection *TextPagerEdit::insertTextSection(int pos, int size, const QTextCharFormat &format, const QVariant &data)
{
    Q_ASSERT(d->document);
    TextPagerSection *section = d->document->insertTextSection(pos, size, format, data);
    if (section) {
        section->d.textEdit = this;
        section->d.priority = 100;
    }
    return section;
}

void TextEditPrivate::updateHorizontalPosition()
{
    const QRect r = textEdit->viewport()->rect();
    const QPoint p(qBound(0, lastMouseMove.x(), r.right()),
                   qBound(0, lastMouseMove.y(), r.bottom()));
    int pos = textPositionAt(p);
    if (pos == -1)
        pos = document->documentSize() - 1;
    textEdit->setCursorPosition(pos, TextPagerCursor::KeepAnchor);
}

void TextEditPrivate::updateScrollBarPosition()
{
    if (requestedScrollBarPosition == -1) {
        return;
    } else if (requestedScrollBarPosition == viewportPosition) {
        requestedScrollBarPosition = -1;
        return;
    }

    const int req = requestedScrollBarPosition;
    requestedScrollBarPosition = -1;

    Direction direction = Forward;
    if (lastRequestedScrollBarPosition != -1 && lastRequestedScrollBarPosition != req) {
        if (lastRequestedScrollBarPosition > req) {
            direction = Backward;
        }
    } else if (req < viewportPosition) {
        direction = Backward;
    }

    lastRequestedScrollBarPosition = req;

    updateViewportPosition(req, direction);
}

void TextEditPrivate::updateScrollBarPageStep()
{
    if (lines.isEmpty()) {
        textEdit->verticalScrollBar()->setPageStep(1);
        return;
    }
    const int visibleCharacters = lines.at(qMin(visibleLines, lines.size() - 1)).first - lines.at(0).first;
    textEdit->verticalScrollBar()->setPageStep(visibleCharacters);
}

void TextEditPrivate::onDocumentDestroyed()
{
    document = 0;
    textEdit->setDocument(new TextPagerDocument(this)); // there should always be a document
}

void TextEditPrivate::scrollLines(int lines)
{
    int pos = viewportPosition;
    const Direction d = (lines < 0 ? Backward : Forward);
    const int add = lines < 0 ? -1 : 1;

    while (pos + add >= 0 && pos + add < document->documentSize()) {
        if (d == Forward &&
            (lines - add == 0) &&
            bufferReadCharacter(pos) == '\n') {
            // When iterating forwards, be sure not to skip over blank lines
            // (ie. lines containing only '\n') by just ignoring them here
            // - updateViewportPosition automatically takes us one index past
            // this newline, thus displaying the next line)
            break;
        } else {
            pos += add;
            if (bufferReadCharacter(pos) == '\n') {
                if ((lines -= add) == 0) {
                    break;
                }
            }
        }
    }
    updateViewportPosition(pos, d);
}

void TextEditPrivate::timerEvent(QTimerEvent *e)
{
    if (e->timerId() == tripleClickTimer.timerId()) {
        tripleClickTimer.stop();
    } else if (e->timerId() == autoScrollTimer.timerId()) {
        if (pendingTimeOut != -1) {
            autoScrollTimer.start(pendingTimeOut, this);
            pendingTimeOut = -1;
        }
        const QRect r = textEdit->viewport()->rect();
        Q_ASSERT(!r.contains(lastMouseMove));
        enum { LineCount = 1 };
        if (lastMouseMove.y() < r.top()) {
            scrollLines(-LineCount);
            if (atBeginning())
                autoScrollTimer.stop();
        } else {
            Q_ASSERT(lastMouseMove.y() > r.bottom());
            scrollLines(LineCount);
            if (atEnd())
                autoScrollTimer.stop();
        }
        const QPoint p(qBound(0, lastMouseMove.x(), r.right()),
                       qBound(0, lastMouseMove.y(), r.bottom()));
        int pos = textPositionAt(p);
        if (pos == -1)
            pos = document->documentSize() - 1;
        textEdit->setCursorPosition(pos, TextPagerCursor::KeepAnchor);
    } else if (e->timerId() == cursorBlinkTimer.timerId()) {
        cursorVisible = !cursorVisible;
        const QRect r = textEdit->cursorRect(textCursor) & textEdit->viewport()->rect();
        if (!r.isNull())
            textEdit->viewport()->update(r);
    } else {
        Q_ASSERT(0);
    }
}

static inline bool compareTextSectionByPriority(const TextPagerSection *left, const TextPagerSection *right)
{
    return left->priority() > right->priority();
}

void TextEditPrivate::updateCursorPosition(const QPoint &pos)
{
    lastHoverPos = pos;
    const int textPos = textPositionAt(pos);
    if (textPos != -1) {
        QList<TextPagerSection*> hovered = textEdit->sections(textPos, 1, TextPagerSection::IncludePartial);
        qSort(hovered.begin(), hovered.end(), compareTextSectionByPriority);
        Q_FOREACH(TextPagerSection *section, hovered) {
            if (section->hasCursor()) {
                delete sectionCursor;
                sectionCursor = new QCursor(section->cursor());
                textEdit->viewport()->setCursor(*sectionCursor);
                return;
            }
        }
    }
    // no cursor
    if (sectionCursor) {
        textEdit->viewport()->setCursor(Qt::IBeamCursor);
        delete sectionCursor;
        sectionCursor = 0;
    }

    //To notify the line num area widge!
    Q_EMIT scrollBarChanged();
}

bool TextEditPrivate::isSectionOnScreen(const TextPagerSection *section) const
{
    Q_ASSERT(section->document() == document);
    return (::matchSection(section, textEdit)
            && section->position() <= layoutEnd
            && section->position() + section->size() >= viewportPosition);
}

void TextEditPrivate::cursorMoveKeyEventReadOnly(QKeyEvent *e)
{
    if (e == QKeySequence::MoveToNextLine) {
        scrollLines(1);
    } else if (e == QKeySequence::MoveToPreviousLine) {
        scrollLines(-1);
    } else if (e == QKeySequence::MoveToStartOfDocument) {
        textEdit->verticalScrollBar()->setValue(0);
    } else if (e == QKeySequence::MoveToEndOfDocument) {
        textEdit->verticalScrollBar()->setValue(textEdit->verticalScrollBar()->maximum());
    } else if (e == QKeySequence::MoveToNextPage) {
        scrollLines(qMax(1, visibleLines - 1));
    } else if (e == QKeySequence::MoveToPreviousPage) {
        scrollLines(-qMax(1, visibleLines));
    } else if (e == QKeySequence::MoveToStartOfLine) {
         textCursor.movePosition(TextPagerCursor::StartOfLine);
         e->accept();
         return;
    } else if (e == QKeySequence::MoveToEndOfLine) {
         textCursor.movePosition(TextPagerCursor::EndOfLine);
         e->accept();
         return;
    }

    else {
        e->ignore();
        return;
    }
    e->accept();
    textCursor.setPosition(viewportPosition);
}

void TextEditPrivate::relayout()
{
    const QSize s = textEdit->viewport()->size();

    int widestOri=widest;

    widest=-1;
    relayoutByGeometry(s.height());
    if(widest == -1)
        widest=widestOri;

    textEdit->horizontalScrollBar()->setPageStep(s.width());
    textEdit->horizontalScrollBar()->setMaximum(qMax(0, widest - s.width()));
#ifdef _UI_TEXTPAGER_DEBUG
    qDebug() << widest << s.width() << textEdit->horizontalScrollBar()->maximum();
#endif
}

bool TextEditPrivate::dirtyForSection(TextPagerSection *section)
{
    if (isSectionOnScreen(section)) {
        layoutDirty = true;
        textEdit->viewport()->update();
        return true;
    } else {
        return false;
    }
}

void TextEditPrivate::onTextSectionFormatChanged(TextPagerSection *section)
{
    dirtyForSection(section);
}

void TextEditPrivate::onTextSectionCursorChanged(TextPagerSection *section)
{
    if (isSectionOnScreen(section)) {
        updateCursorPosition(lastHoverPos);
    }
}

void TextEditPrivate::onSelectionChanged()
{
    if (inMouseEvent && textCursor.hasSelection() && QApplication::clipboard()->supportsSelection()) {
        textEdit->copy(QClipboard::Selection);
    }

    textEdit->viewport()->update();
    // ### could figure out old selection rect and | it with new one
    // ### and update that but is it worth it?
    updateCopyAndCutEnabled();
}

bool TextEditPrivate::canInsertFromMimeData(const QMimeData *data) const
{
    return data->hasText();
}


//Find the viewport position for the end of the document supposing the the last line 
//is exactly located at the bottom of the viewport.
int TextEditPrivate::findLastPageSize() const
{
    if (!document || document->documentSize() == 0)
        return -1;

#ifdef DEBUG_TEXTPAGER_LASTPAGESIZE
    qDebug() << "TextEditPrivate::findLastPageSize -->";
#endif
    TextEditPrivate p(textEdit);
    p.font=textEdit->font();
    p.viewportPosition = 0;
    p.document = document;
    const int documentSize = document->documentSize();
    p.maxViewportPosition = documentSize;

    //The viewport height (including the height of the horizontal slider)
    int vh=p.textEdit->viewport()->height(); //-p.textEdit->horizontalScrollBar()->height();

    //Max 1.5*MinimumBufferSize can be kept in the buffer after the viewportPosition
    int maxCharNum=p.MinimumBufferSize*1.5-200;

    //We use the lastpage cache to find a better start position

#ifdef DEBUG_TEXTPAGER_LASTPAGESIZE
    qDebug() << "  lastPageCache height:" << lastPage.height << "position:"  <<   lastPage.position <<
                "documentSize:" << lastPage.documentSize << "widest:" << lastPage.widest;
#endif

    if(lastPage.documentSize != documentSize)
    {
        lastPage.clear();
        lastPage.documentSize = documentSize;
    }

    Direction sizeChangeDir=Forward;
    if(lastPage.height != -1) {
        if(lastPage.height > vh) {
            maxCharNum=documentSize-lastPage.position+1;
            sizeChangeDir=Backward;
        } else if(lastPage.height< vh) {
            maxCharNum=documentSize-lastPage.position+1000;
            sizeChangeDir=Backward;
        } else {
            return lastPage.position;
        }
    }

    lastPage.height=vh;

    Q_ASSERT(maxCharNum>=0);

    int start = documentSize-maxCharNum;
    if(start < 0) start=0;
    int i = 0;
#ifdef DEBUG_TEXTPAGER_LASTPAGESIZE 
    QTime tm;
    tm.start();
#endif    
    
    //Try to find the last page by iteration
    int step=0;
    const int maxStep=10;
    bool reachedTop=(start == 0);

    while(step < maxStep) {
#ifdef DEBUG_TEXTPAGER_LASTPAGESIZE
        qDebug() << "  ITERATION STEP:" << step;
        qDebug() << "  start:" << start << "maxCharNum:" << maxCharNum;
#endif
        //We get the viewportPosition closest to start
        p.updateViewportPosition(start,sizeChangeDir,false);

#ifdef DEBUG_TEXTPAGER_LASTPAGESIZE
        qDebug() << "  after updateviewport> start-viewportPos:" << start-p.viewportPosition;
#endif
        //adjust the max number of characters
        if(start > p.viewportPosition)
            maxCharNum+=(start-p.viewportPosition)+1;

        //Relayout using the max number of characters
        p.relayoutByPosition(maxCharNum);

#ifdef DEBUG_TEXTPAGER_LASTPAGESIZE     
        qDebug() << "  vh:" << vh << "fontSize:" << font.pointSize();
        qDebug() << "  viewport size" << textEdit->viewport()->size();
        qDebug() << "  layoutEnd:" << p.layoutEnd << "documentSize:" << documentSize << "viewportPosition:" << p.viewportPosition << p.contentRect;
        qDebug() << "  bottom:" << p.textLayouts.last()->boundingRect().bottom();
#endif  
   
        //The end of the layout should be the end of the document
        if(p.layoutEnd == documentSize && p.textLayouts.count() >0) {
   
            int top=p.textLayouts.last()->boundingRect().bottom()+4-vh;
            int pos=documentSize-p.textLayouts.last()->text().size();
           
            for(int i=p.textLayouts.count()-2; i >=0; i--) {
          
                if(p.textLayouts.at(i)->boundingRect().top() <= top) {
#ifdef DEBUG_TEXTPAGER_LASTPAGESIZE 
                    qDebug() << "  find position:" << pos; // << "line:" << p.document->lineNumber(pos);
#endif
                    p.updateViewportPosition(pos, Backward,false);
#ifdef DEBUG_TEXTPAGER_LASTPAGESIZE 
                    qDebug() << "  viewPortPosition:" << p.viewportPosition << "time:" << tm.elapsed(); // << "line:" << p.document->lineNumber(p.viewportPosition);
#endif
                    lastPage.position=p.viewportPosition;
                    lastPage.widest=p.widest;
                    return p.viewportPosition;
                }
                pos-=p.textLayouts.at(i)->text().size();
            }
        }
        //We could not find the last page. We try again from a larger number of characters from the
        //end of the document
        step++;
        maxCharNum+=1000;
        start = documentSize-maxCharNum;

        if(start < 0 && reachedTop)
            break;

        if(start < 0)
        {
            reachedTop=true;
            start=0;
        }
    }

    lastPage.clear();

    //the whole text is in the viewport, no vertical scrollbar is needed
    if(reachedTop)
        return -1;

    //If we are here we the last page position will be the end of the document and there will be
    //a white space block after the last row in the editor.
    //TODO: improve it!
    return documentSize;
}

void TextPagerEdit::setSyntaxHighlighter(SyntaxHighlighter *h)
{
    if (h && h->textEdit() == this) {
        if (d->syntaxHighlighters.size() == 1)
            return;
        takeSyntaxHighlighter(h);
    }
    clearSyntaxHighlighters();
    if (h) {
        addSyntaxHighlighter(h);
    }
}


void TextPagerEdit::setEnableSearchHighlighter(bool b)
{
    useSearchHighlight_=b;
    if(useSearchHighlight_) {
    	setSyntaxHighlighter(searchHighlight_);
    } else {
    	//searchHighlight_ can only be deleted when the editor is deleted.
    	//Because clearSyntaxHighlighters() deletes the highlighters  we need to use
    	//takeSyntaxHighlighter() instead.
        if(d->syntaxHighlighters.contains(searchHighlight_)) {
            takeSyntaxHighlighter(searchHighlight_);
            d->layoutDirty = true;
            viewport()->update();
            //clearSyntaxHighlighters();
        }
    }
}

void TextPagerEdit::clearSearchHighlighter()
{
    searchHighlight_->clear();
}

void TextPagerEdit::setSearchHighlighter(QString txt,TextPagerDocument::FindMode mode)
{
    searchHighlight_->reset(txt,mode,useSearchHighlight_);
}

void TextPagerEdit::setSearchHighlighter(QRegExp rx,TextPagerDocument::FindMode mode)
{
    searchHighlight_->reset(rx,mode,useSearchHighlight_);
}

void TextPagerEdit::gotoLine(int lineNum)
{
    TextPagerCursor cursor=d->document->findLine(lineNum,textCursor());
    if(!cursor.isNull())
        setTextCursor(cursor);
}

//---------------------------------------------
// Fontsize management
//---------------------------------------------

void TextPagerEdit::setFontProperty(VProperty* p)
{
	fontProp_=p;
	fontProp_->addObserver(this);
	updateFont();
}

void TextPagerEdit::zoomIn()
{
	QFont f=font();
	int fps=f.pointSize();
	f.setPointSize(fps+1);
	setFont(f);

	fontSizeChangedByZoom();
}

void TextPagerEdit::zoomOut()
{
	int oriSize=font().pointSize();

	QFont f=font();
	int fps=f.pointSize();
	if(fps > 1)
	{
		f.setPointSize(fps-1);
		setFont(f);
	}

	if(font().pointSize() != oriSize)
		fontSizeChangedByZoom();
}

void TextPagerEdit::fontSizeChangedByZoom()
{
	if(fontProp_)
		fontProp_->setValue(font());
}

void TextPagerEdit::updateFont()
{
	if(fontProp_)
	{
		QFont f=fontProp_->value().value<QFont>();
		if(font() != f)
			setFont(f);
	}
}

void TextPagerEdit::notifyChange(VProperty* p)
{
	if(fontProp_ ==p)
	{
		setFont(p->value().value<QFont>());
	}
}

void TextPagerEdit::setLineNumberArea(TextPagerLineNumberArea *a)
{
	lineNumArea_=a;
	connect(d,SIGNAL(scrollBarChanged()),
			lineNumArea_,SLOT(update()));

    //Initialise the width
    lineNumArea_->updateWidth();
}

void TextPagerEdit::lineNumberAreaPaintEvent(QPaintEvent *e)
{
    if(!lineNumArea_)
    	return;

    QPainter painter(lineNumArea_);
    const QRect er = e->rect();
    //painter.translate(-horizontalScrollBar()->value(), 0);
    //painter.setFont(font());

    QVector<QTextLayout::FormatRange> selections;
    selections.reserve(d->extraSelections.size() + 1);
    int textLayoutOffset = d->viewportPosition;

    QRect numRect=er;

    //Background and border
    painter.fillRect(numRect, lineNumArea_->bgColour());
    painter.setPen(QPen(lineNumArea_->separatorColour()));
    painter.drawLine(lineNumArea_->width()-1,er.y(),lineNumArea_->width()-1,er.bottom());

    int numWidth=lineNumArea_->width()-lineNumArea_->rightMargin();

    QFont fontNormal(font());   // the font to use for most line numbers
    QFont fontBold(fontNormal);  // the font to use for the current line number
    fontBold.setBold(true);
    painter.setPen(lineNumArea_->fontColour());
    painter.setFont(fontNormal);

    int cursorPos=d->textCursor.position();

    //const QTextLayout *cursorLayout = d->cursorVisible ? d->layoutForPosition(d->textCursor.position()) : 0;
    //int extraSelectionIndex = 0;
    QTextLayout::FormatRange selectionRange;
    selectionRange.start = -1;
    int maxLineNum=-1;
    Q_FOREACH(QTextLayout *l, d->textLayouts) {
        const int textSize = l->text().size();
        const QRect r = l->boundingRect().toRect();
        if (r.intersects(er)) {

        	const int lineNum=lineNumber(textLayoutOffset)+1;
            maxLineNum=lineNum;
            QRect lRect(0,r.y(),numWidth,r.height());

        	// is this the current line?
        	if(cursorPos >= textLayoutOffset && cursorPos <= textLayoutOffset+l->text().size())
        	{
        		painter.setFont(fontBold);
                painter.fillRect(lRect, lineNumArea_->currentColour());  // highlight the background
        	    painter.drawText(lRect,QString::number(lineNum),Qt::AlignRight|Qt::AlignVCenter);
        	    painter.setFont(fontNormal);
        	}
        	else
        	{        		
        		painter.drawText(lRect,QString::number(lineNum),Qt::AlignRight|Qt::AlignVCenter);
        	}

        } else if (r.top() > er.bottom()) {
            break;
        }
        textLayoutOffset += textSize + 1;
    }

    if(maxLineNum != -1)
        lineNumArea_->updateWidth(maxLineNum);
}

//==========================================================================
//
// TextPagerLineNumberArea
//
//==========================================================================

TextPagerLineNumberArea::TextPagerLineNumberArea(TextPagerEdit *editor) :
    QWidget(editor), textEditor_ (editor), digits_(6), rightMargin_(3),
    bgCol_(232,231,230), fontCol_(220,220,200), separatorCol_(220,220,200),
    currentCol_(212,212,255)
{
    Q_ASSERT(textEditor_);

    if(VProperty* p=VConfig::instance()->find("view.textEdit.numAreaBackground"))
        bgCol_=p->value().value<QColor>();

    if(VProperty* p=VConfig::instance()->find("view.textEdit.numAreaFontColour"))
        fontCol_=p->value().value<QColor>();

    if(VProperty* p=VConfig::instance()->find("view.textEdit.numAreaSeparator"))
        separatorCol_=p->value().value<QColor>();

    if(VProperty* p=VConfig::instance()->find("view.textEdit.numAreaCurrent"))
        currentCol_=p->value().value<QColor>();

    editor->setLineNumberArea(this);
}

void TextPagerLineNumberArea::updateWidth(int maxLineNum)
{
    if(maxLineNum == -1 || pow(10,digits_)-1 < maxLineNum)
    {
        setFixedWidth(computeWidth(maxLineNum));
    }
}

int TextPagerLineNumberArea::computeWidth(int maxLineNum) const
{
    if(maxLineNum > 0)
    {
        int maxDigits=1;
        int mx = maxLineNum;
        while ( mx >= 10)
        {
           mx /= 10;
           ++maxDigits;
        }

        if(maxDigits > digits_)
            digits_=maxDigits;
    }

    return  (textEditor_)?(3 + textEditor_->fontMetrics().width(QLatin1Char('9')) * digits_ + rightMargin_):3+rightMargin_;

}




