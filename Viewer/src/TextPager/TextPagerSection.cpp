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

#include "TextPagerSection.hpp"
#include "TextPagerDocument.hpp"
#include "TextPagerDocument_p.hpp"

TextPagerSection::~TextPagerSection()
{
    if (d.document)
        d.document->takeTextSection(this);
}

QString TextPagerSection::text() const
{
    Q_ASSERT(d.document);
    return d.document->read(d.position, d.size);
}

void TextPagerSection::setFormat(const QTextCharFormat &format)
{
    Q_ASSERT(d.document);
    d.format = format;
    Q_EMIT d.document->d->sectionFormatChanged(this);
}

QCursor TextPagerSection::cursor() const
{
    return d.cursor;
}

void TextPagerSection::setCursor(const QCursor &cursor)
{
    d.cursor = cursor;
    d.hasCursor = true;
    Q_EMIT d.document->d->sectionCursorChanged(this);
}

void TextPagerSection::resetCursor()
{
    d.hasCursor = false;
    d.cursor = QCursor();
    Q_EMIT d.document->d->sectionCursorChanged(this);
}

bool TextPagerSection::hasCursor() const
{
    return d.hasCursor;
}

void TextPagerSection::setPriority(int priority)
{
    d.priority = priority;
    Q_EMIT d.document->d->sectionFormatChanged(this); // ### it hasn't really but I to need it dirtied
}
