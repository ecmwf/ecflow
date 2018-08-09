// Copyright 2010 Anders Bakken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0

// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef SYNTAXHIGHLIGHTER_HPP__
#define SYNTAXHIGHLIGHTER_HPP__

#include <QObject>
#include <QString>
#include <QTextCharFormat>
#include <QColor>
#include <QFont>
#include <QList>
#include <QTextLayout>

class TextPagerEdit;
class TextPagerLayout;
class TextPagerDocument;
class SyntaxHighlighter : public QObject
{
    Q_OBJECT
public:
    SyntaxHighlighter(QObject *parent = nullptr);
    SyntaxHighlighter(TextPagerEdit *parent);
    ~SyntaxHighlighter() override;
    void setTextEdit(TextPagerEdit *doc);
    TextPagerEdit *textEdit() const;
    TextPagerDocument *document() const;
    virtual void highlightBlock(const QString &text) = 0;
    QString currentBlock() const { return d->currentBlock; }
    void setFormat(int start, int count, const QTextCharFormat &format);
    void setFormat(int start, int count, const QColor &color);
    inline void setColor(int start, int count, const QColor &color)
    { setFormat(start, count, color); }
    inline void setBackground(int start, int count, const QBrush &brush)
    { QTextCharFormat format; format.setBackground(brush); setFormat(start, count, format); }
    inline void setBackgroundColor(int start, int count, const QColor &color)
    { setBackground(start, count, color); }
    void setFormat(int start, int count, const QFont &font);
    inline void setFont(int start, int count, const QFont &font)
    { setFormat(start, count, font); }
    QTextBlockFormat blockFormat() const;
    void setBlockFormat(const QTextBlockFormat &format);
    QTextCharFormat format(int pos) const;
    int previousBlockState() const;
    int currentBlockState() const;
    void setCurrentBlockState(int s);
    int currentBlockPosition() const;
public Q_SLOTS:
    void rehighlight();
private:
    struct Private {
        Private() = default;
        TextPagerEdit *textEdit{nullptr};
        TextPagerLayout *textLayout{nullptr};
        int previousBlockState{0}, currentBlockState{0}, currentBlockPosition{-1};
        QList<QTextLayout::FormatRange> formatRanges;
        QTextBlockFormat blockFormat;
        QString currentBlock;
    } *d;

    friend class TextPagerEdit;
    friend class TextPagerLayout;
};


#endif
