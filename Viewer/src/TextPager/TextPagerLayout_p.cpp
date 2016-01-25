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

#include "TextPagerLayout_p.hpp"
#include "TextPagerEdit_p.hpp"
#include "TextPagerDocument.hpp"
#include "TextPagerEdit.hpp"

int TextPagerLayout::viewportWidth() const
{
    if (!lineBreaking)
        return INT_MAX - 1024;
    return textEdit ? textEdit->viewport()->width() : viewport;
}

int TextPagerLayout::doLayout(int index, QList<TextPagerSection*> *sections) // index is in document coordinates
{
    QTextLayout *textLayout = 0;
    if (!unusedTextLayouts.isEmpty()) {
        textLayout = unusedTextLayouts.takeLast();
        textLayout->clearAdditionalFormats();
    } else {
        textLayout = new QTextLayout;
        textLayout->setCacheEnabled(true);
        textLayout->setFont(font);
        QTextOption option;
        option.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
        textLayout->setTextOption(option);
    }
    textLayouts.append(textLayout);
    if (index != 0 && bufferReadCharacter(index - 1) != '\n') {
        qWarning() << index << viewportPosition << document->read(index - 1, 20)
                   << bufferReadCharacter(index - 1);
    }
    Q_ASSERT(index == 0 || bufferReadCharacter(index - 1) == '\n');
    const int max = bufferPosition + buffer.size();
    const int lineStart = index;
    while (index < max && bufferReadCharacter(index) != '\n')
        ++index;

    const QString string = buffer.mid(lineStart - bufferPosition, index - lineStart);
    Q_ASSERT(string.size() == index - lineStart);
    Q_ASSERT(!string.contains('\n'));
    if (index < max)
        ++index; // for the newline
    textLayout->setText(string);

    QMultiMap<int, QTextLayout::FormatRange> formatMap;
    if (sections) {
        do {
            Q_ASSERT(!sections->isEmpty());
            TextPagerSection *l = sections->first();
            Q_ASSERT(::matchSection(l, textEdit));
            Q_ASSERT(l->position() + l->size() >= lineStart);
            if (l->position() >= index) {
                break;
            }
            // section is in this QTextLayout
            QTextLayout::FormatRange range;
            range.start = qMax(0, l->position() - lineStart); // offset in QTextLayout
            range.length = qMin(l->position() + l->size(), index) - lineStart - range.start;
            range.format = l->format();
            formatMap.insertMulti(l->priority(), range);
            if (l->position() + l->size() >= index) { // > ### ???
                // means section didn't end here. It continues in the next QTextLayout
                break;
            }
            sections->removeFirst();
        } while (!sections->isEmpty());
    }
    QList<QTextLayout::FormatRange> formats = formatMap.values();

    int leftMargin = LeftMargin;
    int rightMargin = 0;
    int topMargin = 0;
    int bottomMargin = 0;
    Q_FOREACH(SyntaxHighlighter *syntaxHighlighter, syntaxHighlighters) {
        syntaxHighlighter->d->currentBlockPosition = lineStart;
        syntaxHighlighter->d->formatRanges.clear();
        syntaxHighlighter->d->currentBlock = string;
        syntaxHighlighter->highlightBlock(string);
        syntaxHighlighter->d->currentBlock.clear();
        if (syntaxHighlighter->d->blockFormat.isValid()) {
            blockFormats[textLayout] = syntaxHighlighter->d->blockFormat;
            if (syntaxHighlighter->d->blockFormat.hasProperty(QTextFormat::BlockLeftMargin))
                leftMargin = syntaxHighlighter->d->blockFormat.leftMargin();
            if (syntaxHighlighter->d->blockFormat.hasProperty(QTextFormat::BlockRightMargin))
                rightMargin = syntaxHighlighter->d->blockFormat.rightMargin();
            if (syntaxHighlighter->d->blockFormat.hasProperty(QTextFormat::BlockTopMargin))
                topMargin = syntaxHighlighter->d->blockFormat.topMargin();
            if (syntaxHighlighter->d->blockFormat.hasProperty(QTextFormat::BlockBottomMargin))
                bottomMargin = syntaxHighlighter->d->blockFormat.bottomMargin();

        }
        syntaxHighlighter->d->previousBlockState = syntaxHighlighter->d->currentBlockState;
        if (!syntaxHighlighter->d->formatRanges.isEmpty())
            formats += syntaxHighlighter->d->formatRanges;
    }
    textLayout->setAdditionalFormats(formats);
    textLayout->beginLayout();
    const int lineWidth = viewportWidth() - (leftMargin + rightMargin);

    int localWidest = -1;
    Q_FOREVER {
        QTextLine line = textLayout->createLine();
        if (!line.isValid()) {
            break;
        }
        line.setLineWidth(lineWidth);
        if (!lineBreaking)
            localWidest = qMax<int>(localWidest, line.naturalTextWidth() + (LeftMargin * 2));
        // ### support blockformat margins etc
        int y = topMargin + lastBottomMargin;
        if (!lines.isEmpty()) {
            y += int(lines.last().second.rect().bottom());
            // QTextLine doesn't seem to get its rect() update until a
            // new line has been created (or presumably in endLayout)
        }
        line.setPosition(QPoint(leftMargin, y));
        lines.append(qMakePair(lineStart + line.textStart(), line));
    }
    widest = qMax(widest, localWidest);
    lastBottomMargin = bottomMargin;

    textLayout->endLayout();
#ifndef QT_NO_DEBUG
    for (int i=1; i<lines.size(); ++i) {
        Q_ASSERT(lines.at(i).first - (lines.at(i - 1).first + lines.at(i - 1).second.textLength()) <= 1);
    }
#endif


    QRect r = textLayout->boundingRect().toRect();
    // this will actually take the entire width set in setLineWidth
    // and not what it actually uses.
    r.setWidth(localWidest);

    contentRect |= r;
    Q_ASSERT(!lineBreaking || contentRect.right() <= qint64(viewportWidth()) + LeftMargin
             || viewportWidth() == -1);

    Q_FOREACH(SyntaxHighlighter *syntaxHighlighter, syntaxHighlighters) {
        syntaxHighlighter->d->formatRanges.clear();
        syntaxHighlighter->d->blockFormat = QTextBlockFormat();
        syntaxHighlighter->d->currentBlockPosition = -1;
    }

    return index;
}

int TextPagerLayout::textPositionAt(const QPoint &p) const
{
    QPoint pos = p;
    if (pos.x() >= 0 && pos.x() < LeftMargin)
        pos.rx() = LeftMargin; // clicking in the margin area should count as the first characters

    int textLayoutOffset = viewportPosition;
    Q_FOREACH(const QTextLayout *l, textLayouts) {
    	if(l->boundingRect().y() <= pos.y() && l->boundingRect().bottom() >=pos.y()) {
    	//if (l->boundingRect().toRect().contains(pos)) {
            const int lineCount = l->lineCount();
            for (int i=0; i<lineCount; ++i) {
                const QTextLine line = l->lineAt(i);
                if (line.y() <= pos.y() && pos.y() <= line.height() + line.y()) { // ### < ???
                {
                	if(pos.x() > l->boundingRect().right())
                		pos.setX(l->boundingRect().right()-1);

                	return textLayoutOffset + line.xToCursor(qMax<int>(LeftMargin, pos.x()));
                }
		}
            }
        }
        textLayoutOffset += l->text().size() + 1; // + 1 for newlines which aren't in the QTextLayout
    }
    return -1;
}

QList<TextPagerSection*> TextPagerLayout::relayoutCommon()
{
//    widest = -1; // ### should this be relative to current content or remember? What if you remove the line that was the widest?
    Q_ASSERT(layoutDirty);
    layoutDirty = false;
    Q_ASSERT(document);
    lines.clear();
    unusedTextLayouts = textLayouts;
    textLayouts.clear();
    contentRect = QRect();
    visibleLines = lastVisibleCharacter = -1;

    Q_FOREACH(SyntaxHighlighter *syntaxHighlighter, syntaxHighlighters) {
        syntaxHighlighter->d->previousBlockState = syntaxHighlighter->d->currentBlockState = -1;
    }

    if (viewportPosition < bufferPosition
        || (bufferPosition + buffer.size() < document->documentSize()
            && buffer.size() - bufferOffset() < MinimumBufferSize)) {
        bufferPosition = qMax(0, viewportPosition - MinimumBufferSize);
        buffer = document->read(bufferPosition, int(MinimumBufferSize * 2.5));
        sections = document->d->getSections(bufferPosition, buffer.size(), TextPagerSection::IncludePartial, textEdit);
    } else if (sectionsDirty) {
        sections = document->d->getSections(bufferPosition, buffer.size(), TextPagerSection::IncludePartial, textEdit);
    }
    sectionsDirty = false;
    QList<TextPagerSection*> l = sections;
    while (!l.isEmpty() && l.first()->position() + l.first()->size() < viewportPosition)
        l.takeFirst(); // could cache these as well
    return l;
}

void TextPagerLayout::relayoutByGeometry(int height)
{
    if (!layoutDirty)
        return;

    QList<TextPagerSection*> l = relayoutCommon();

    const int max = viewportPosition + buffer.size() - bufferOffset(); // in document coordinates
    ASSUME(viewportPosition == 0 || bufferReadCharacter(viewportPosition - 1) == '\n');

    static const int extraLines = qMax(2, qgetenv("LAZYTEXTEDIT_EXTRA_LINES").toInt());
    int index = viewportPosition;
    while (index < max) {
        index = doLayout(index, l.isEmpty() ? 0 : &l);
        Q_ASSERT(index == max || document->readCharacter(index - 1) == '\n');
        Q_ASSERT(!textLayouts.isEmpty());
        const int y = int(textLayouts.last()->boundingRect().bottom());
        if (y >= height) {
            if (visibleLines == -1) {
                visibleLines = lines.size();
                lastVisibleCharacter = index;
            } else if (lines.size() >= visibleLines + extraLines) {
                break;
            }
        }
    }
    if (visibleLines == -1) {
        visibleLines = lines.size();
        lastVisibleCharacter = index;
    }


    layoutEnd = qMin(index, max);
    qDeleteAll(unusedTextLayouts);
    unusedTextLayouts.clear();
    Q_ASSERT(viewportPosition < layoutEnd ||
             (viewportPosition == layoutEnd && viewportPosition == document->documentSize()));
//    qDebug() << "layoutEnd" << layoutEnd << "viewportPosition" << viewportPosition;
}

void TextPagerLayout::relayoutByPosition(int size)
{
    if (!layoutDirty)
        return;

    QList<TextPagerSection*> l = relayoutCommon();

    const int max = viewportPosition + qMin(size, buffer.size() - bufferOffset());
    Q_ASSERT(viewportPosition == 0 || bufferReadCharacter(viewportPosition - 1) == '\n');
    int index = viewportPosition;
    while (index < max) {
        index = doLayout(index, l.isEmpty() ? 0 : &l);
    }
    layoutEnd = index;

    qDeleteAll(unusedTextLayouts);
    unusedTextLayouts.clear();
    Q_ASSERT(viewportPosition < layoutEnd ||
             (viewportPosition == layoutEnd && viewportPosition == document->documentSize()));
}

void TextPagerLayout::relayout()
{
    //relayoutByPosition(2000); // ### totally arbitrary number
    relayoutByPosition(1.5*MinimumBufferSize); 
}

QTextLayout *TextPagerLayout::layoutForPosition(int pos, int *offset, int *index) const
{
    if (offset)
        *offset = -1;
    if (index)
        *index = -1;

    if (textLayouts.isEmpty() || pos < viewportPosition || pos > layoutEnd) {
        return 0;
    }

    int textLayoutOffset = viewportPosition;
    int i = 0;

    Q_FOREACH(QTextLayout *l, textLayouts) {
        if (pos >= textLayoutOffset && pos <= l->text().size() + textLayoutOffset) {
            if (offset)
                *offset = pos - textLayoutOffset;
            if (index)
                *index = i;
            return l;
        }
        ++i;
        textLayoutOffset += l->text().size() + 1;
    }
    return 0;
}

QTextLine TextPagerLayout::lineForPosition(int pos, int *offsetInLine, int *lineIndex, bool *lastLine) const
{
    if (offsetInLine)
        *offsetInLine = -1;
    if (lineIndex)
        *lineIndex = -1;
    if (lastLine)
        *lastLine = false;

    if (pos < viewportPosition || pos >= layoutEnd || textLayouts.isEmpty() || lines.isEmpty()) {
        return QTextLine();
    }
    int layoutIndex = 0;
    QTextLayout *layout = textLayouts.value(layoutIndex);
    Q_ASSERT(layout);
    for (int i=0; i<lines.size(); ++i) {
        const QPair<int, QTextLine> &line = lines.at(i);
        int lineEnd = line.first + line.second.textLength();
        const bool last = line.second.lineNumber() + 1 == layout->lineCount();
        if (last) {
            ++lineEnd;
            // 1 is for newline characters
            layout = textLayouts.value(++layoutIndex);
            // could be 0
        }
        if (pos < lineEnd) {
            if (offsetInLine) {
                *offsetInLine = pos - line.first;
                Q_ASSERT(*offsetInLine >= 0);
                Q_ASSERT(*offsetInLine < lineEnd + pos);
            }
            if (lineIndex) {
                *lineIndex = i;
            }
            if (lastLine)
                *lastLine = last;
            return line.second;
        } else if (!layout) {
            break;
        }
    }
    qWarning() << "Couldn't find a line for" << pos << "viewportPosition" << viewportPosition
               << "layoutEnd" << layoutEnd;
    Q_ASSERT(0);
    return QTextLine();
}

// pos is not necessarily on a newline. Finds closest newline in the
// right direction and sets viewportPosition to that. Updates
// scrollbars if this is a TextEditPrivate

void TextPagerLayout::updateViewportPosition(int pos, Direction direction,bool applyIt)
{
    pos = qMin(pos, maxViewportPosition);
    if (document->documentSize() == 0) {
        viewportPosition = 0;
    } else {
        Q_ASSERT(document->documentSize() > 0);
        int index = document->find('\n', qMax(0, pos + (direction == Backward ? -1 : 0)),
                                   TextPagerDocument::FindMode(direction)).anchor();
        if (index == -1) {
            if (direction == Backward) {
                index = 0;
            } else {
                index = qMax(0, document->find('\n', document->documentSize() - 1, TextPagerDocument::FindBackward).position());
                // position after last newline in document
                // if there is no newline put it at 0
            }
        } else {
            ++index;
        }
        Q_ASSERT(index != -1);
        viewportPosition = index;

        if (viewportPosition != 0 && document->read(viewportPosition - 1, 1) != QString("\n"))
            qWarning() << "viewportPosition" << viewportPosition << document->read(viewportPosition - 1, 10) << this;
        ASSUME(viewportPosition == 0 || document->read(viewportPosition - 1, 1) == QString("\n"));
    }
    if (viewportPosition > maxViewportPosition && direction == Forward) {
        updateViewportPosition(viewportPosition, Backward);
        return;
    }
    layoutDirty = true;

    if(applyIt) {
    
        if (textEdit && !suppressTextEditUpdates) {
            textEdit->viewport()->update();
            TextEditPrivate *p = static_cast<TextEditPrivate*>(this);
            p->pendingScrollBarUpdate = true;
            p->updateCursorPosition(p->lastHoverPos);
            if (!textEdit->verticalScrollBar()->isSliderDown()) {
                p->updateScrollBar();
            } // sliderReleased is connected to updateScrollBar()
        }
    
        relayout();
    }    
}

#ifndef QT_NO_DEBUG_STREAM
QDebug &operator<<(QDebug &str, const QTextLine &line)
{
    if (!line.isValid()) {
        str << "QTextLine() (invalid)";
        return str;
    }
    str.space() << "lineNumber" << line.lineNumber()
                << "textStart" << line.textStart()
                << "textLength" << line.textLength()
                << "position" << line.position();
    return str;
}

QString TextPagerLayout::dump() const
{
    QString out;
    QTextStream ts(&out, QIODevice::WriteOnly);
    ts << "viewportPosition " << viewportPosition
       << " layoutEnd " << layoutEnd
       << " viewportWidth " << viewportWidth() << '\n';
    for (int i=0; i<textLayouts.size(); ++i) {
        QTextLayout *layout = textLayouts.at(i);
        for (int j=0; j<layout->lineCount(); ++j) {
            QTextLine line = layout->lineAt(j);
            ts << layout->text().mid(line.textStart(), line.textLength());
            if (j + 1 < layout->lineCount()) {
                ts << "<linebreak>\n";
            } else {
                ts << "\n";
            }
        }
    }
    return out;
}

#endif
