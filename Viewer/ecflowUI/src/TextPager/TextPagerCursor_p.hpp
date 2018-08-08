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

#ifndef TEXTPAGERCURSOR_P_HPP__
#define TEXTPAGERCURSOR_P_HPP__

#include <QSize>
#include <QList>
#include <QObject>
#include <QHash>
#include <QAtomicInt>
#include <QCoreApplication>
#include <QTextCursor>
#include "TextPagerCursor_p.hpp"
#include "TextPagerEdit.hpp"
#include "TextPagerEdit_p.hpp"

static inline bool match(const TextPagerCursor &cursor, const TextPagerLayout *layout, int lines)
{
    Q_ASSERT(cursor.document() == layout->document);
    if (cursor.viewportWidth() != -1 && cursor.viewportWidth() != layout->viewport) {
        return false;
    }
    if (layout->viewportPosition > cursor.position()
        || layout->layoutEnd < cursor.position()) {
        return false;
    }
    int index = -1;
    if (!layout->layoutForPosition(cursor.position(), nullptr, &index)) {
        // ### need an interface for this if I am going to have a mode
        // ### that doesn't break lines
        return false;
    }

    if (index < lines && layout->viewportPosition > 0) {
        return false;  // need more margin before the cursor
    } else if (layout->textLayouts.size() - index - 1 < lines && layout->layoutEnd < layout->document->documentSize()) {
        return false; // need more margin after cursor
    }
    return true;
}

class TextLayoutCacheManager : public QObject
{
    Q_OBJECT
public:
    static TextLayoutCacheManager *instance()
    {
        static auto *inst = new TextLayoutCacheManager(QCoreApplication::instance());
        return inst;
    }
    // ### this class doesn't react to TextSections added or removed. I
    // ### don't think it needs to since it's only being used for
    // ### cursor movement which shouldn't be impacted by these things

    static TextPagerLayout *requestLayout(const TextPagerCursor &cursor, int margin)
    {
        Q_ASSERT(cursor.document());
        if (cursor.textEdit && match(cursor, cursor.textEdit->d, margin)) {
            return cursor.textEdit->d;
        }

        TextPagerDocument *doc = cursor.document();
        Q_ASSERT(doc);
        QList<TextPagerLayout*> &layouts = instance()->cache[doc];
        Q_ASSERT(cursor.document());
        Q_FOREACH(TextPagerLayout *l, layouts) {
            if (match(cursor, l, margin)) {
                return l;
            }
        }
        if (layouts.size() < instance()->maxLayouts) {
            if (layouts.isEmpty()) {
                connect(cursor.document(), SIGNAL(charactersAdded(int, int)),
                        instance(), SLOT(onCharactersAddedOrRemoved(int)));
                connect(cursor.document(), SIGNAL(charactersRemoved(int, int)),
                        instance(), SLOT(onCharactersAddedOrRemoved(int)));
                connect(cursor.document(), SIGNAL(destroyed(QObject*)),
                        instance(), SLOT(onDocumentDestroyed(QObject*)));
            }
            layouts.append(new TextPagerLayout(doc));
        }
        TextPagerLayout *l = layouts.last();
        l->viewport = cursor.viewportWidth();
        if (l->viewport == -1)
            l->viewport = INT_MAX - 1024; // prevent overflow in comparisons.
        if (cursor.textEdit) {
            l->textEdit = cursor.textEdit;
            l->suppressTextEditUpdates = true;
            l->lineBreaking = cursor.textEdit->lineBreaking();
            l->sections = cursor.textEdit->d->sections;
            l->font = cursor.textEdit->font();
            l->syntaxHighlighters = cursor.textEdit->syntaxHighlighters();
//            l->extraSelections = cursor.textEdit->extraSelections();
            // ### can the extra selections impact layout? If so they
            // ### need to be in the actual textLayout shouldn't need
            // ### to care about the actual selection
        }
        int startPos = (cursor.position() == 0
                        ? 0
                        : qMax(0, doc->find(QLatin1Char('\n'), cursor.position() - 1, TextPagerDocument::FindBackward).anchor()));
        // We start at the beginning of the current line
        int linesAbove = margin;
        if (startPos > 0) {
            while (linesAbove > 0) {
                const TextPagerCursor c = doc->find(QLatin1Char('\n'), startPos - 1, TextPagerDocument::FindBackward);
                if (c.isNull()) {
                    startPos = 0;
                    break;
                }
                startPos = c.anchor();
                ASSUME(c.anchor() == 0 || c.cursorCharacter() == QLatin1Char('\n'));
                ASSUME(c.anchor() == 0 || doc->readCharacter(c.anchor()) == QLatin1Char('\n'));
                ASSUME(c.cursorCharacter() == doc->readCharacter(c.anchor()));

                --linesAbove;
            }
        }

        int linesBelow = margin;
        int endPos = cursor.position();
        if (endPos < doc->documentSize()) {
            while (linesBelow > 0) {
                const TextPagerCursor c = doc->find(QLatin1Char('\n'), endPos + 1);
                if (c.isNull()) {
                    endPos = doc->documentSize();
                    break;
                }
                endPos = c.anchor();
                --linesBelow;
            }
        }
        if (startPos > 0)
            ++startPos; // in this case startPos points to the newline before it
        l->viewportPosition = startPos;
        l->layoutDirty = true;
        ASSUME(l->viewportPosition == 0 || doc->readCharacter(l->viewportPosition - 1) == QLatin1Char('\n'));
        l->relayoutByPosition(endPos - startPos + 100); // ### fudged a couple of lines likely
        ASSUME(l->viewportPosition < l->layoutEnd
               || (l->viewportPosition == l->layoutEnd && l->viewportPosition == doc->documentSize()));
        ASSUME(l->textLayouts.size() > margin * 2 || l->viewportPosition == 0 || l->layoutEnd == doc->documentSize());
        return l;
    }
private Q_SLOTS:
    void onDocumentDestroyed(QObject *o)
    {
        qDeleteAll(cache.take(static_cast<TextPagerDocument*>(o)));
    }

    void onCharactersAddedOrRemoved(int pos)
    {
        QList<TextPagerLayout*> &layouts = cache[qobject_cast<TextPagerDocument*>(sender())];
        ASSUME(!layouts.isEmpty());
        for (int i=layouts.size() - 1; i>=0; --i) {
            TextPagerLayout *l = layouts.at(i);
            if (pos <= l->layoutEnd) {
                delete l;
                layouts.removeAt(i);
            }
        }
        if (layouts.isEmpty()) {
            disconnect(sender(), nullptr, this, nullptr);
            cache.remove(qobject_cast<TextPagerDocument*>(sender()));
        }
    }
private:
    TextLayoutCacheManager(QObject *parent)
        : QObject(parent)
    {
        maxLayouts = qMax(1, qgetenv("TEXTCURSOR_MAX_CACHED_TEXTLAYOUTS").toInt());
    }

    int maxLayouts;
    QHash<TextPagerDocument*, QList<TextPagerLayout*> > cache;
};

class TextPagerLayout;
struct TextCursorSharedPrivate
{
public:
    TextCursorSharedPrivate() : ref(1), position(-1), anchor(-1),
        overrideColumn(-1), viewportWidth(-1),
        document(nullptr)
    {}

    ~TextCursorSharedPrivate()
    {
        int refint = ref.fetchAndAddRelaxed(0); // required to get past a bug in Qt 5.2.1 (see Ubuntu 14.04)
        ASSUME(refint == 0);
    }

    void invalidate()
    {

    }

    mutable QAtomicInt ref;
    int position, anchor, overrideColumn, viewportWidth;

    TextPagerDocument *document;
};


#endif
