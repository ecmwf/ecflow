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

#ifndef TEXTPAGERDOCUMENT_P_HPP__
#define TEXTPAGERDOCUMENT_P_HPP__

#include <QString>
#include <QApplication>
#include <QThread>
#include <QIODevice>
#include <QTime>
#include <QMutexLocker>
#include <QReadWriteLock>
#include <QMutex>
#include <QSet>
#include <QTemporaryFile>
#include <QDebug>
#include <QPointer>

#ifndef ASSUME
#ifdef FATAL_ASSUMES
#define ASSUME(cond) Q_ASSERT(cond)
#elif defined Q_OS_SOLARIS
#define ASSUME(cond) if (!(cond)) qWarning("Failed assumption %s:%d %s", __FILE__, __LINE__, #cond);
#else
#define ASSUME(cond) if (!(cond)) qWarning("Failed assumption %s:%d (%s) %s", __FILE__, __LINE__, __FUNCTION__, #cond);
#endif
#endif

#define Q_COMPARE_ASSERT(left, right) if (left != right) { qWarning() << left << right; Q_ASSERT(left == right); }

#include "TextPagerDocument.hpp"
#ifdef NO_TEXTDOCUMENT_CACHE
#define NO_TEXTDOCUMENT_CHUNK_CACHE
#define NO_TEXTDOCUMENT_READ_CACHE
#endif

#if defined TEXTDOCUMENT_LINENUMBER_CACHE && !defined TEXTDOCUMENT_LINENUMBER_CACHE_INTERVAL
#define TEXTDOCUMENT_LINENUMBER_CACHE_INTERVAL 100
#endif

static inline bool matchSection(const TextPagerSection *section, const TextPagerEdit *textEdit)
{
    if (!textEdit) {
        return true;
    } else if (!section->textEdit()) {
        return true;
    } else {
        return textEdit == section->textEdit();
    }
}


struct Chunk {
    Chunk() : previous(0), next(0), from(-1), length(0), firstLineIndex(-1)
#ifndef TEXTDOCUMENT_LINENUMBER_CACHE
            , lines(-1)
#endif
        {}

    mutable QString data;
    Chunk *previous, *next;
    int size() const { return data.isEmpty() ? length : data.size(); }
#ifdef QT_DEBUG
    int pos() const { int p = 0; Chunk *c = previous; while (c) { p += c->size(); c = c->previous; }; return p; }
#endif
    mutable int from, length; // Not used when all is loaded
    mutable int firstLineIndex;
#ifdef TEXTDOCUMENT_LINENUMBER_CACHE
    mutable QVector<int> lineNumbers;
    // format is how many endlines in the area from (n *
    // TEXTDOCUMENT_LINENUMBER_CACHE_INTERVAL) to
    // ((n + 1) * TEXTDOCUMENT_LINENUMBER_CACHE_INTERVAL)
#else
    mutable int lines;
#endif
    QString swap;
};


// should really use this stuff for all of this stuff

static inline QPair<int, int> intersection(int index1, int size1, int index2, int size2)
{
    QPair<int, int> ret;
    ret.first = qMax(index1, index2);
    const int right = qMin(index1 + size1, index2 + size2);
    ret.second = right - ret.first;
    if (ret.second <= 0)
        return qMakePair(-1, 0);
    return ret;
}

static inline bool compareTextSection(const TextPagerSection *left, const TextPagerSection *right)
{
    // don't make this compare document. Look at ::sections()
    return left->position() < right->position();
}

class TextDocumentIterator;
struct DocumentCommand {
    enum Type {
        None,
        Inserted,
        Removed
    };

    DocumentCommand(Type t, int pos = -1, const QString &string = QString())
        : type(t), position(pos), text(string), joinStatus(NoJoin)
    {}

    const Type type;
    int position;
    QString text;

    enum JoinStatus {
        NoJoin,
        Forward,
        Backward
    } joinStatus;
};

struct TextPagerSection;
struct TextCursorSharedPrivate;
struct TextDocumentPrivate : public QObject
{
    Q_OBJECT
public:
    TextDocumentPrivate(TextPagerDocument *doc)
        : q(doc), first(0), last(0),
#ifndef NO_TEXTDOCUMENT_CHUNK_CACHE
          cachedChunk(0), cachedChunkPos(-1),
#endif
#ifndef NO_TEXTDOCUMENT_READ_CACHE
          cachePos(-1),
#endif
          documentSize(0),
          saveState(NotSaving), findState(NotFinding), ownDevice(false), modified(false),
          deviceMode(TextPagerDocument::Sparse), chunkSize(16384), //chunkSize(1024*64), //chunkSize(16384),
          //undoRedoStackCurrent(0), modifiedIndex(-1), undoRedoEnabled(true), ignoreUndoRedo(false),
          //collapseInsertUndo(false),
		  hasChunksWithLineNumbers(false), textCodec(0), options(TextPagerDocument::DefaultOptions),
          readWriteLock(0), cursorCommand(false)
    {
        first = last = new Chunk;
    }

    TextPagerDocument *q;
    QSet<TextCursorSharedPrivate*> textCursors;
    mutable Chunk *first, *last;

#ifndef NO_TEXTDOCUMENT_CHUNK_CACHE
    mutable Chunk *cachedChunk;
    mutable int cachedChunkPos;
    mutable QString cachedChunkData; // last uninstantiated chunk's read from file
#endif
#ifndef NO_TEXTDOCUMENT_READ_CACHE
    mutable int cachePos;
    mutable QString cache; // results of last read(). Could span chunks
#endif

    int documentSize;
    enum SaveState { NotSaving, Saving, AbortSave } saveState;
    enum FindState { NotFinding, Finding, AbortFind } mutable findState;
    QList<TextPagerSection*> sections;
    QPointer<QIODevice> device;
    bool ownDevice, modified;
    TextPagerDocument::DeviceMode deviceMode;
    int chunkSize;

#if 0
    QList<DocumentCommand*> undoRedoStack;
    int undoRedoStackCurrent, modifiedIndex;
    bool undoRedoEnabled, ignoreUndoRedo, collapseInsertUndo;
#endif

    bool hasChunksWithLineNumbers;
    QTextCodec *textCodec;
    TextPagerDocument::Options options;
    QReadWriteLock *readWriteLock;
    bool cursorCommand;

#ifdef QT_DEBUG
    mutable QSet<TextDocumentIterator*> iterators;
#endif

#if 0
    void joinLastTwoCommands();
#endif

    void removeChunk(Chunk *c);
    QString chunkData(const Chunk *chunk, int pos) const;
    int chunkIndex(const Chunk *c) const;

    // evil API. pos < 0 means don't cache

    void updateChunkLineNumbers(Chunk *c, int pos) const;
    int countNewLines(Chunk *c, int chunkPos, int index) const;

    void instantiateChunk(Chunk *chunk);
    Chunk *chunkAt(int pos, int *offset) const;

#if 0
    void clearRedo();
    void undoRedo(bool undo);
#endif

    QString wordAt(int position, int *start = 0) const;
    QString paragraphAt(int position, int *start = 0) const;

    uint wordBoundariesAt(int pos) const;

    friend class TextPagerDocument;
    void swapOutChunk(Chunk *c);
    QList<TextPagerSection*> getSections(int from, int size, TextPagerSection::TextSectionOptions opt, const TextPagerEdit *filter) const;
    inline TextPagerSection *sectionAt(int pos, const TextPagerEdit *filter) const { return getSections(pos, 1, TextPagerSection::IncludePartial, filter).value(0); }
    void textEditDestroyed(TextPagerEdit *edit);
Q_SIGNALS:
    void sectionFormatChanged(TextPagerSection *section);
    void sectionCursorChanged(TextPagerSection *section);

#if 0
    void undoRedoCommandInserted(DocumentCommand *cmd);
    void undoRedoCommandRemoved(DocumentCommand *cmd);
    void undoRedoCommandTriggered(DocumentCommand *cmd, bool undo);
    void undoRedoCommandFinished(DocumentCommand *cmd);
#endif

private:
    friend class TextPagerSection;
};

// should not be kept as a member. Invalidated on inserts/removes
class TextDocumentIterator
{
public:
    TextDocumentIterator(const TextDocumentPrivate *d, int p)
        : doc(d), pos(p), min(0), max(-1), convert(false), newline('\n')
    {
        Q_ASSERT(doc);

        end_=end();

#ifndef NO_TEXTDOCUMENTITERATOR_CACHE
        chunk = doc->chunkAt(p, &offset);
        Q_ASSERT(chunk);
        const int chunkPos = p - offset;
        chunkData = doc->chunkData(chunk, chunkPos);
        //chunkLines = chunkData.split('\n');
        Q_COMPARE_ASSERT(chunkData.size(), chunk->size());
#ifdef QT_DEBUG
        if (p != d->documentSize) {
            if (doc->q->readCharacter(p) != chunkData.at(offset)) {
                qDebug() << "got" << chunkData.at(offset) << "at" << offset << "in" << chunk << "of size" << chunkData.size()
                         << "expected" << doc->q->readCharacter(p) << "at document pos" << pos;
            }
            Q_ASSERT(chunkData.at(offset) == doc->q->readCharacter(p));
        } else {
            Q_ASSERT(chunkData.size() == offset);
        }
#endif
#endif

#ifdef QT_DEBUG
        doc->iterators.insert(this);
#endif
    }
#ifdef QT_DEBUG
    ~TextDocumentIterator()
    {
        Q_ASSERT(doc->iterators.remove(this));
    }
#endif

    int end() const
    {
        return max != -1 ? max : doc->documentSize;
    }

    void setMinBoundary(int bound)
    {
        min = bound;
        Q_ASSERT(pos >= min);
    }

    void setMaxBoundary(int bound)
    {
        max = bound;
        Q_ASSERT(pos <= max);

        //We suppose that the document size does not change
        end_=end();
    }


    inline bool hasNext() const
    {
        return pos < end();
    }

    inline bool hasPrevious() const
    {
        return pos > min;
    }

    inline int position() const
    {
        return pos;
    }

    inline QChar current() const
    {
        Q_ASSERT(doc);
#ifndef NO_TEXTDOCUMENTITERATOR_CACHE
        Q_ASSERT(chunk);
        Q_COMPARE_ASSERT(chunkData.size(), chunk->size());
        if (pos == end())
            return QChar();

//Calling readCharacter is very expensive during find!!! So we try not to call it!!!
#if 0
#ifdef QT_DEBUG
        if (doc->q->readCharacter(pos) != chunkData.at(offset)) {
            qDebug() << "got" << chunkData.at(offset) << "at" << offset << "in" << chunk << "of size" << chunkData.size()
                     << "expected" << doc->q->readCharacter(pos) << "at document pos" << pos;
        }
#endif
#endif
#if 0
        ASSUME(doc->q->readCharacter(pos) == chunkData.at(offset));
#endif
        return convert ? chunkData.at(offset).toLower() : chunkData.at(offset);
        //return convert ? chunkData[offset].toLower() : chunkData[offset];
#else
        return convert ? doc->q->readCharacter(pos).toLower() : doc->q->readCharacter(pos);
#endif
    }

    inline QChar next()
    {
        Q_ASSERT(doc);
        ++pos;
#ifndef NO_TEXTDOCUMENTITERATOR_CACHE
        Q_ASSERT(chunk);
        if (++offset >= chunkData.size() && chunk->next) { // special case for offset == chunkData.size() and !chunk->next
            offset = 0;
            chunk = chunk->next;
            Q_ASSERT(chunk);
            chunkData = doc->chunkData(chunk, pos);
            Q_COMPARE_ASSERT(chunkData.size(), chunk->size());
        }
#endif
        return current();
    }

    inline QChar previous()
    {
        Q_ASSERT(doc);
        Q_ASSERT(hasPrevious());
        --pos;
        Q_ASSERT(pos >= min);
        Q_ASSERT(pos <= end());
#ifndef NO_TEXTDOCUMENTITERATOR_CACHE
        Q_ASSERT(chunk);
        if (--offset < 0) {
            chunk = chunk->previous;
            Q_ASSERT(chunk);
            chunkData = doc->chunkData(chunk, pos - chunk->size() + 1);
            Q_COMPARE_ASSERT(chunkData.size(), chunk->size());
            offset = chunkData.size() - 1;
        }
#endif
        return current();
    }

    enum Direction { None = 0x0, Left = 0x1, Right = 0x2, Both = Left|Right };
    inline QChar nextPrev(Direction dir, bool &ok)
    {
        if (dir == Left) {
            if (!hasPrevious()) {
                ok = false;
                return QChar();
            }
            ok = true;
            return previous();
        } else {
            if (!hasNext()) {
                ok = false;
                return QChar();
            }
            ok = true;
            return next();
        }
    }

// Gets the previous line preceding the current iterator position. It is only
// used for TextPagerDocument::find and was added to speed up the original find
// algorithm. it is only implemented for cached textdocument iterators!

    inline unsigned char prevLine(QString& str)
    {
#ifndef NO_TEXTDOCUMENTITERATOR_CACHE
       Q_ASSERT(doc);

#ifdef QT_DEBUG
    	//qDebug() << pos << offset;  //<< chunkData;
#endif

    	//Get the QString in the chunk as a const pointer. It is faster to iterate through it than
    	//calling QString::at()
    	const QChar *data = chunkData.constData();
    	data+=offset;

    	//The current character is probably a newline (the
    	//result of the previous call) so we need to take a step back
    	if(*data == newline)
    	{
    	   //If we are at the start
    	   if(pos <= min) {
		   //if(!hasPrevious()) {
    		   return 0;
    	   }

    	   --pos;

    	   //See if we need the previous chunk
    	   if(--offset < 0) {
    	       loadPrevChunk();
    	       data = chunkData.constData();
    	   } else {
    		   --data;
    	   }
    	}

    	//Mark the line end position within the chunk
    	int to=offset;

    	//We will go backwards until we find a newline
    	while(*data != newline)
        {
#ifdef QT_DEBUG
        	//qDebug() << pos << offset << chunkData.at(offset) << to;
#endif
    		if(pos <= min) {
    			if(to != offset) {
    			   if(str.size() == 0) {
    			       str=chunkData.mid(offset,to-offset+1);
    			   } else {
    			       str.prepend(chunkData.mid(offset,to-offset+1));
    			   }
    			}
        		return 0;
        	}

        	--pos;

        	//If we need the previous chunk
        	if(--offset < 0) {

        		//Copy the line portion from this chunk to the result
        		if(str.size() == 0)
        			str=chunkData.mid(0,to);
        		else
        			str.prepend(chunkData.mid(0,to));

        		//Get previous chunk
        		loadPrevChunk();

        		//Initialise the search positions
        		data = chunkData.constData();
        		data+=offset;
        		to=offset;
        	} else {
        		--data;
        	}
        }

#ifdef QT_DEBUG
        	//qDebug() << pos << offset << to;
#endif

        //offset is either a newline charter or 0 (the start of the document)

    	if(to != offset) {
    	  if(str.size() == 0) {
    	       str=chunkData.mid(offset,to-offset+1);
    	   } else {
    	       str.prepend(chunkData.mid(offset,to-offset+1));
    	  }
        }
#endif

#ifdef QT_DEBUG
    	//qDebug() << "line:" << str;
#endif

        return 1;
    }

    // Gets the next line following- the current iterator position. It is only
    // used for TextPagerDocument::find and was added to speed up the original find
    // algorithm. it is only implemented for cached textdocument iterators!

    inline unsigned char nextLine(QString& str)
    {
#ifndef NO_TEXTDOCUMENTITERATOR_CACHE
        Q_ASSERT(doc);

#ifdef QT_DEBUG
    //    	qDebug() << pos << offset;  //<< chunkData;
#endif

        //Get the QString in the chunk as a const pointer. It is faster to iterate through it than
        //calling QString::at()
        const QChar *data = chunkData.constData();
        data+=offset;

        //The current character is probably a newline (the
        //result of the previous call) so we need to take a step forward
        if(*data == newline)
        {
        	//If we are at the end
        	if(pos >= end())
        	//if(!)hasNext())
        		return 0;

        	++pos;

        	//See if we need the next chunk
        	if(++offset >= chunkData.size()) {
        		loadNextChunk();
        	    data = chunkData.constData();

        	} else {
        		++data;
        	}
        }

        //Mark the line start position within the chunk
        int from=offset;

        //We will go forward until we find a newline
        while(*data != newline)
        {
#ifdef QT_DEBUG
        	//qDebug() << pos << offset << chunkData.at(offset);
#endif
        	if(pos >= end())
        	//if(!hasNext())
        		return 0;

            ++pos;

            //If we need the next chunk
            if(++offset >= chunkData.size()) {

            	//Copy the line portion from this chink to the result
            	if(str.size() == 0)
            		str=chunkData.mid(from,offset-from);
            	else
            		str.append(chunkData.mid(from,offset-from));

            	//Get next chunk
            	loadNextChunk();

            	//Initialise the search positions
            	data = chunkData.constData();
            	from=0;
            }
            else {
            	++data;
            }

        }

        if(from != offset) {
            if(str.size() == 0)
            {
            	str=chunkData.mid(from,offset-from+1);
            }
            else
            	str.append(chunkData.mid(from,offset-from+1));
        }

#endif

#ifdef QT_DEBUG
        	//qDebug() << "line:" << str;
#endif

        return 1;
    }


    inline void loadPrevChunk()
    {
#ifndef NO_TEXTDOCUMENTITERATOR_CACHE
        Q_ASSERT(chunk);
        if (chunk->previous) {
            chunk = chunk->previous;
            Q_ASSERT(chunk);
            chunkData = doc->chunkData(chunk, pos - chunk->size() + 1);
            Q_COMPARE_ASSERT(chunkData.size(), chunk->size());
            offset = chunkData.size() - 1;
        }
#endif
    }

    inline void loadNextChunk()
    {
#ifndef NO_TEXTDOCUMENTITERATOR_CACHE
        Q_ASSERT(chunk);
        if (chunk->next) {
            chunk = chunk->next;
            Q_ASSERT(chunk);
            chunkData = doc->chunkData(chunk, pos);
            Q_COMPARE_ASSERT(chunkData.size(), chunk->size());
            offset = 0;
        }
#endif
    }


    inline bool convertToLowerCase() const
    {
        return convert;
    }

    inline void setConvertToLowerCase(bool on)
    {
        convert = on;
    }

private:
    const TextDocumentPrivate *doc;
    int pos;
    int min, max;
    int end_; // to speed things up
#ifndef NO_TEXTDOCUMENTITERATOR_CACHE
    int offset;
    QString chunkData;
    QStringList chunkLines;
    Chunk *chunk;
#endif
    bool convert;
    const QChar newline;
};


#endif
