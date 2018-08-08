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

#ifndef TEXTPAGERSECTION_HPP__
#define TEXTPAGERSECTION_HPP__

#include <QString>
#include <QTextCharFormat>
#include <QVariant>
#include <QCursor>

class TextPagerDocument;
class TextPagerEdit;
class TextPagerSection
{
public:
    enum TextSectionOption {
        IncludePartial = 0x01
    };
    Q_DECLARE_FLAGS(TextSectionOptions, TextSectionOption);

    ~TextPagerSection();
    QString text() const;
    int position() const { return d.position; }
    int size() const { return d.size; }
    QTextCharFormat format() const { return d.format; }
    void setFormat(const QTextCharFormat &format);
    QVariant data() const { return d.data; }
    void setData(const QVariant &data) { d.data = data; }
    TextPagerDocument *document() const { return d.document; }
    TextPagerEdit *textEdit() const { return d.textEdit; }
    QCursor cursor() const;
    void setCursor(const QCursor &cursor);
    void resetCursor();
    bool hasCursor() const;
    int priority() const { return d.priority; }
    void setPriority(int priority);
private:
    struct Data {
        Data(int p, int s, TextPagerDocument *doc, const QTextCharFormat &f, const QVariant &d)
            : position(p), size(s), priority(0), document(doc), textEdit(nullptr), format(f), data(d), hasCursor(false)
        {}
        int position, size, priority;
        TextPagerDocument *document;
        TextPagerEdit *textEdit;
        QTextCharFormat format;
        QVariant data;
        QCursor cursor;
        bool hasCursor;
    } d;

    TextPagerSection(int pos, int size, TextPagerDocument *doc, const QTextCharFormat &format, const QVariant &data)
        : d(pos, size, doc, format, data)
    {}

    friend class TextPagerDocument;
    friend class TextDocumentPrivate;
    friend class TextPagerEdit;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(TextPagerSection::TextSectionOptions);


#endif
