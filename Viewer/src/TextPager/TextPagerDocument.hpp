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

#ifndef TEXTPAGERDOCUMENT_HPP__
#define TEXTPAGERDOCUMENT_HPP__

#include <QObject>
#include <QString>
#include <QString>
#include <QPair>
#include <QEventLoop>
#include <QList>
#include <QVariant>
#include <QTextCharFormat>
#include <QTextCodec>
#include <QChar>
#include <QRegExp>

#include "TextPagerCursor.hpp"
#include "TextPagerSection.hpp"

class Chunk;
class TextDocumentPrivate;
class TextPagerDocument : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int documentSize READ documentSize)
    Q_PROPERTY(int chunkCount READ chunkCount)
    Q_PROPERTY(int instantiatedChunkCount READ instantiatedChunkCount)
    Q_PROPERTY(int swappedChunkCount READ swappedChunkCount)
    Q_PROPERTY(int chunkSize READ chunkSize WRITE setChunkSize)
    Q_ENUMS(DeviceMode)
    Q_FLAGS(Options)
    Q_FLAGS(FindMode)

public:
    TextPagerDocument(QObject *parent = 0);
    ~TextPagerDocument();

    enum DeviceMode {
        Sparse,
        LoadAll
    };

    enum Option {
        NoOptions = 0x0000,
        SwapChunks = 0x0001,
        KeepTemporaryFiles = 0x0002,
        ConvertCarriageReturns = 0x0004, // incompatible with Sparse and must be set before loading
        AutoDetectCarriageReturns = 0x0010,
        NoImplicitLoadAll = 0x0020,
        Locking = 0x0040,
        DefaultOptions = AutoDetectCarriageReturns
    };
    Q_DECLARE_FLAGS(Options, Option);

    Options options() const;
    void setOptions(Options opt);
    inline void setOption(Option opt, bool on = true) { setOptions(on ? (options() | opt) : (options() &= ~opt)); }

    inline bool load(QIODevice *device, DeviceMode mode, const QByteArray &codecName)
    { return load(device, mode, QTextCodec::codecForName(codecName)); }
    inline bool load(const QString &fileName, DeviceMode mode, const QByteArray &codecName)
    { return load(fileName, mode, QTextCodec::codecForName(codecName)); }
    bool load(QIODevice *device, DeviceMode mode = Sparse, QTextCodec *codec = 0);
    bool load(const QString &fileName, DeviceMode mode = Sparse, QTextCodec *codec = 0);

    void clear();
    DeviceMode deviceMode() const;

    QTextCodec *textCodec() const;

    void setText(const QString &text);
    QString read(int pos, int size) const;
    QStringRef readRef(int pos, int size) const;
    QChar readCharacter(int index) const;

    int documentSize() const;
    int chunkCount() const;
    int instantiatedChunkCount() const;
    int swappedChunkCount() const;

    void lockForRead();
    void lockForWrite();
    bool tryLockForRead();
    bool tryLockForWrite();
    void unlock();

    enum FindModeFlag {
        FindNone = 0x00000,
        FindBackward = 0x00001,
        FindCaseSensitively = 0x00002,
        FindWholeWords = 0x00004,
        FindAllowInterrupt = 0x00008,
        FindWrap = 0x00010,
        FindAll = 0x00020
    };
    Q_DECLARE_FLAGS(FindMode, FindModeFlag);

    int chunkSize() const;
    void setChunkSize(int pos);

    QIODevice *device() const;

    TextPagerCursor find(const QRegExp &rx, const TextPagerCursor &cursor, FindMode flags = 0) const;
    TextPagerCursor find(const QString &ba, const TextPagerCursor &cursor, FindMode flags = 0) const;
    TextPagerCursor find(const QChar &ch, const TextPagerCursor &cursor, FindMode flags = 0) const;

    inline TextPagerCursor find(const QRegExp &rx, int pos = 0, FindMode flags = 0) const
    { return find(rx, TextPagerCursor(this, pos), flags); }
    inline TextPagerCursor find(const QString &ba, int pos = 0, FindMode flags = 0) const
    { return find(ba, TextPagerCursor(this, pos), flags); }
    inline TextPagerCursor find(const QChar &ch, int pos = 0, FindMode flags = 0) const
    { return find(ch, TextPagerCursor(this, pos), flags); }


    QList<TextPagerSection*> sections(int from = 0, int size = -1, TextPagerSection::TextSectionOptions opt = 0) const;
    inline TextPagerSection *sectionAt(int pos) const { return sections(pos, 1, TextPagerSection::IncludePartial).value(0); }
    TextPagerSection *insertTextSection(int pos, int size, const QTextCharFormat &format = QTextCharFormat(),
                                   const QVariant &data = QVariant());
    void insertTextSection(TextPagerSection *section);
    void takeTextSection(TextPagerSection *section);
    int currentMemoryUsage() const;

    bool isModified() const;

    int lineNumber(int position) const;
    int columnNumber(int position) const;
    int lineNumber(const TextPagerCursor &cursor) const;
    int columnNumber(const TextPagerCursor &cursor) const;
    virtual bool isWordCharacter(const QChar &ch, int index) const;

public Q_SLOTS:
    bool abortSave();
    bool abortFind() const;

Q_SIGNALS:
    void entryFound(const TextPagerCursor &cursor) const;
    void textChanged();
    void sectionAdded(TextPagerSection *section);
    void sectionRemoved(TextPagerSection *removed);
    void charactersAdded(int from, int count);
    void charactersRemoved(int from, int count);
    void saveProgress(qreal progress);
    void findProgress(qreal progress, int position) const;
    void documentSizeChanged(int size);
    void modificationChanged(bool modified);

protected:
    virtual QString swapFileName(Chunk *chunk);

private:
    TextDocumentPrivate *d;
    friend class TextPagerEdit;
    friend class TextPagerCursor;
    friend class TextDocumentPrivate;
    friend class TextPagerLayout;
    friend class TextPagerSection;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(TextPagerDocument::FindMode);
Q_DECLARE_OPERATORS_FOR_FLAGS(TextPagerDocument::Options);

#endif
