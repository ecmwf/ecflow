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

#ifndef TEXTPAGEREDIT_P_HPP__
#define TEXTPAGEREDIT_P_HPP__

#include <QBasicTimer>
#include <QPoint>
#include <QAction>
#include <QScrollArea>
#include <QScrollBar>
#include "TextPagerLayout_p.hpp"
#include "TextPagerDocument_p.hpp"
#include "TextPagerCursor.hpp"
#include "TextPagerEdit.hpp"

struct DocumentCommand;
struct CursorData {
    int position, anchor;
};

struct LastPageCache {
    LastPageCache() : position(-1),height(-1),widest(-1), documentSize(-1) {}
    void clear() {position=-1; height=-1; widest=-1; documentSize=-1;}
    int position;
    int height;
    int widest;
    int documentSize;
};

class TextEditPrivate : public QObject, public TextPagerLayout
{
    Q_OBJECT
public:
    TextEditPrivate(TextPagerEdit *qptr)
        : requestedScrollBarPosition(-1), lastRequestedScrollBarPosition(-1), cursorWidth(2),
        sectionCount(0), maximumSizeCopy(50000), pendingTimeOut(-1), autoScrollLines(0),
        readOnly(true), cursorVisible(false), blockScrollBarUpdate(false),
        updateScrollBarPageStepPending(true), inMouseEvent(false), sectionPressed(nullptr),
        pendingScrollBarUpdate(false), sectionCursor(nullptr)
    {
        textEdit = qptr;
    }

    bool canInsertFromMimeData(const QMimeData *data) const;
    void updateHorizontalPosition();
    void updateScrollBarPosition();
    void updateScrollBarPageStep();
    void scrollLines(int lines);
    void timerEvent(QTimerEvent *e) override;
    void updateCursorPosition(const QPoint &pos);
    int findLastPageSize() const;
    bool atBeginning() const { return viewportPosition == 0; }
    bool atEnd() const { return textEdit->verticalScrollBar()->value() == textEdit->verticalScrollBar()->maximum(); }
    bool dirtyForSection(TextPagerSection *section);
    void updateCopyAndCutEnabled();
    bool isSectionOnScreen(const TextPagerSection *section) const;
    void cursorMoveKeyEventReadOnly(QKeyEvent *e);
    void relayout() override; // from TextPagerLayout
    void adjustVerticalScrollBar();
    
    int requestedScrollBarPosition, lastRequestedScrollBarPosition, cursorWidth, sectionCount,
        maximumSizeCopy, pendingTimeOut, autoScrollLines;
    bool readOnly, cursorVisible, blockScrollBarUpdate, updateScrollBarPageStepPending, inMouseEvent;
    QBasicTimer autoScrollTimer, cursorBlinkTimer;
    QAction *actions[TextPagerEdit::SelectAllAction];
    TextPagerSection *sectionPressed;
    TextPagerCursor textCursor, dragOverrideCursor;
    QBasicTimer tripleClickTimer;
    bool pendingScrollBarUpdate;
    QCursor *sectionCursor;
    QPoint lastHoverPos, lastMouseMove;
    QHash<DocumentCommand *, QPair<CursorData, CursorData> > undoRedoCommands;
    mutable LastPageCache lastPage;

public Q_SLOTS:
    void onSyntaxHighlighterDestroyed(QObject *o);
    void onSelectionChanged();
    void onTextSectionAdded(TextPagerSection *section);
    void onTextSectionRemoved(TextPagerSection *section);
    void onTextSectionFormatChanged(TextPagerSection *section);
    void onTextSectionCursorChanged(TextPagerSection *section);
    void updateScrollBar();
    void onDocumentDestroyed();
    void onDocumentSizeChanged(int size);

#if 0
    void onDocumentCommandInserted(DocumentCommand *cmd);
    void onDocumentCommandFinished(DocumentCommand *cmd);
    void onDocumentCommandRemoved(DocumentCommand *cmd);
    void onDocumentCommandTriggered(DocumentCommand *cmd, bool undo);
#endif


    void onScrollBarValueChanged(int value);
    void onScrollBarActionTriggered(int action);
    void onCharactersAddedOrRemoved(int index, int count);

Q_SIGNALS:
    void scrollBarChanged();
};

/*
class DebugWindow : public QWidget
{
public:
    DebugWindow(TextEditPrivate *p)
        : priv(p)
    {
    }

    void paintEvent(QPaintEvent *)
    {
        if (priv->lines.isEmpty())
            return;

        QPainter p(this);
        p.fillRect(QRect(0, pixels(priv->viewportPosition), width(),
                         pixels(priv->lines.last().first +
                                priv->lines.last().second.textLength())),
                   Qt::black);
        p.fillRect(QRect(0, pixels(priv->viewportPosition), width(), pixels(priv->layoutEnd)), Qt::red);
    }

    int pixels(int pos) const
    {
        double fraction = double(pos) / double(priv->document->documentSize());
        return int(double(height()) * fraction);
    }
private:
    TextEditPrivate *priv;
};
*/

#endif
