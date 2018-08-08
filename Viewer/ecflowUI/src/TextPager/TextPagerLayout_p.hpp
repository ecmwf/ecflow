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

#ifndef TEXTPAGERLAYOUT_P_HPP_
#define TEXTPAGERLAYOUT_P_HPP_

#include <QList>
#include <QTextLayout>
#include <QRect>
#include <QString>
#include <QSize>
#include <QTextLine>
#include <QKeyEvent>
#ifndef QT_NO_DEBUG_STREAM
#include <QDebug>
#endif
#include "TextPagerDocument.hpp"
#include "TextPagerEdit.hpp"
#include "syntaxhighlighter.hpp"
#include "WeakPointer.hpp"

#ifndef QT_NO_DEBUG_STREAM
QDebug &operator<<(QDebug &str, const QTextLine &line);
#endif

class TextDocumentBuffer
{
public:
    TextDocumentBuffer(TextPagerDocument *doc) : document(doc), bufferPosition(0) {}
    virtual ~TextDocumentBuffer() = default;

    inline QString bufferRead(int from, int size) const
    {
        Q_ASSERT(document);
        if (from < bufferPosition || from + size > bufferPosition + buffer.size()) {
            return document->read(from, size);
        }
        return buffer.mid(from - bufferPosition, size);
    }
    inline QChar bufferReadCharacter(int index) const // document coordinates
    {
        Q_ASSERT(document);
        if (index >= bufferPosition && index < bufferPosition + buffer.size()) {
            return buffer.at(index - bufferPosition);
        } else {
            Q_ASSERT(index >= 0 && index < document->documentSize()); // what if index == documentSize?
            return document->readCharacter(index);
        }
    }

    TextPagerDocument *document;
    int bufferPosition;
    QString buffer;
};

class TextPagerEdit;
class TextPagerLayout : public TextDocumentBuffer
{
public:
    enum { MinimumBufferSize = 90000, LeftMargin = 3 };
    TextPagerLayout(TextPagerDocument *doc = nullptr)
        : TextDocumentBuffer(doc), textEdit(nullptr),
        viewportPosition(0), layoutEnd(-1), viewport(-1),
        visibleLines(-1), lastVisibleCharacter(-1), lastBottomMargin(0),
        widest(-1), maxViewportPosition(0), layoutDirty(true), sectionsDirty(true),
        lineBreaking(false), suppressTextEditUpdates(false)
    {
    }

    virtual ~TextPagerLayout()
    {
        qDeleteAll(textLayouts);
        qDeleteAll(unusedTextLayouts);
    }

    TextPagerEdit *textEdit;
    QList<SyntaxHighlighter*> syntaxHighlighters;
    int viewportPosition, layoutEnd, viewport, visibleLines,
        lastVisibleCharacter, lastBottomMargin, widest, maxViewportPosition;
    bool layoutDirty, sectionsDirty, lineBreaking, suppressTextEditUpdates;
    QList<QTextLayout*> textLayouts, unusedTextLayouts;
    QHash<QTextLayout*, QTextBlockFormat> blockFormats;
    QList<TextPagerEdit::ExtraSelection> extraSelections;
    QList<QPair<int, QTextLine> > lines; // int is start position of line in document coordinates
    QRect contentRect; // contentRect means the laid out area, not just the area currently visible
    QList<TextPagerSection*> sections; // these are all the sections in the buffer. Some might be before the current viewport
    QFont font;

    QList<TextPagerSection*> relayoutCommon(); // should maybe be smarter about MinimumScreenSize. Detect it based on font and viewport size
    void relayoutByPosition(int size);
    void relayoutByGeometry(int height);
    virtual void relayout();

    int viewportWidth() const;

    int doLayout(int index, QList<TextPagerSection*> *sections);

    QTextLine lineForPosition(int pos, int *offsetInLine = nullptr,
                              int *lineIndex = nullptr, bool *lastLine = nullptr) const;
    QTextLayout *layoutForPosition(int pos, int *offset = nullptr, int *index = nullptr) const;

    int textPositionAt(const QPoint &pos) const;
    inline int bufferOffset() const { return viewportPosition - bufferPosition; }

    QString dump() const;

    enum Direction {
        Forward = 0,
        Backward = TextPagerDocument::FindBackward
    };
    void updateViewportPosition(int pos, Direction direction,bool applyIt=true);
};

#endif
