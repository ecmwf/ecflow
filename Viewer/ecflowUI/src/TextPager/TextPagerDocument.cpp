#include "TextPagerDocument.hpp"
#include "TextPagerCursor.hpp"
#include "TextPagerCursor_p.hpp"
#include "TextPagerDocument_p.hpp"
#include <QBuffer>
#include <QIODevice>
#include <QObject>
#include <QString>
#include <QString>
#include <QFile>
#include <QFileInfo>
#include <QList>
#include <QTextCharFormat>
#include <QVariant>
#include <QDesktopServices>
#include <qalgorithms.h>

//#define TEXTDOCUMENT_FIND_DEBUG
#define TEXTDOCUMENT_USE_FILE_LOCK


#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
 #include <QStandardPaths>
#endif

// #define DEBUG_CACHE_HITS

#ifndef TEXTDOCUMENT_FIND_INTERVAL_PERCENTAGE
#define TEXTDOCUMENT_FIND_INTERVAL_PERCENTAGE 1000
#endif

#ifndef TEXTDOCUMENT_MAX_INTERVAL
#define TEXTDOCUMENT_MAX_INTERVAL 1000
#endif

#ifdef TEXTDOCUMENT_FIND_SLEEP
static void findSleep(const TextPagerDocument *document)
{
    Q_ASSERT(document);
    const int duration = document->property("TEXTDOCUMENT_FIND_SLEEP").toInt();
    if (duration > 0) {
        usleep(duration * 1000);
    }
}
#endif

TextPagerDocument::TextPagerDocument(QObject *parent)
    : QObject(parent), d(new TextDocumentPrivate(this))
{
}

TextPagerDocument::~TextPagerDocument()
{
    {
        QWriteLocker locker(d->readWriteLock);
        Q_FOREACH(TextCursorSharedPrivate *cursor, d->textCursors) {
            cursor->document = 0;
        }
        Chunk *c = d->first;
        while (c) {
            if (!(d->options & KeepTemporaryFiles) && !c->swap.isEmpty())
                QFile::remove(c->swap);
            Chunk *tmp = c;
            c = c->next;
            delete tmp;
        }
        Q_FOREACH(TextPagerSection *section, d->sections) {
            section->d.document = 0;
            section->d.textEdit = 0;
            delete section;
        }
        if (d->ownDevice)
            delete d->device.data();
    }
    delete d->readWriteLock;
    delete d;
}

bool TextPagerDocument::load(QIODevice *device, DeviceMode mode, QTextCodec *codec)
{
    QWriteLocker locker(d->readWriteLock);
    Q_ASSERT(device);
    if (!device->isReadable())
        return false;

    Options options = d->options;
    if (options & ConvertCarriageReturns && mode == Sparse) {
        qWarning("TextPagerDocument::load() ConvertCarriageReturns is incompatible with Sparse");
        options &= ~ConvertCarriageReturns;
    }
    if ((options & (AutoDetectCarriageReturns|ConvertCarriageReturns)) == (AutoDetectCarriageReturns|ConvertCarriageReturns))
        options &= ~AutoDetectCarriageReturns;

    Q_FOREACH(TextPagerSection *section, d->sections) {
        Q_EMIT sectionRemoved(section);
        section->d.document = 0;
        delete section;
    }
    d->sections.clear();

    if (d->documentSize > 0) {
        Q_EMIT charactersRemoved(0, d->documentSize);
    }

    Chunk *c = d->first;
    while (c) {
        Chunk *tmp = c;
        c = c->next;
        delete tmp;
    }

    d->textCodec = codec;
    d->documentSize = device->size();
    if (d->documentSize <= d->chunkSize && mode == Sparse && !(options & NoImplicitLoadAll))
        mode = LoadAll;
#if 0
    if (codec && mode == Sparse) {

        qWarning("Sparse mode doesn't really work with unicode data yet. I am working on it.\n--\nAnders");
    }
#endif
    d->first = d->last = 0;

    if (d->device) {
        if (d->ownDevice && d->device.data() != device) // this is done when saving to the same file
            delete d->device.data();
    }

    d->ownDevice = false;
    d->device = device;
    d->deviceMode = mode;
#ifndef NO_TEXTDOCUMENT_CHUNK_CACHE
    d->cachedChunk = 0;
    d->cachedChunkPos = -1;
    d->cachedChunkData.clear();
#endif
#ifndef NO_TEXTDOCUMENT_READ_CACHE
    d->cachePos = -1;
    d->cache.clear();
#endif

    switch (d->deviceMode) {
    case LoadAll: {
        device->seek(0);
        QTextStream ts(device);
        if (d->textCodec)
            ts.setCodec(d->textCodec);
        Chunk *current = 0;
        d->documentSize = 0; // in case of unicode
        do {
            Chunk *c = new Chunk;
            c->data = ts.read(d->chunkSize);
            if (options & AutoDetectCarriageReturns) {
                if (c->data.contains(QLatin1Char('\n'))) {
                    options |= ConvertCarriageReturns;
                }
                options &= ~AutoDetectCarriageReturns;
            }
            if (options & ConvertCarriageReturns)
                c->data.remove(QLatin1Char('\r'));
            d->documentSize += c->data.size();
            if (current) {
                current->next = c;
                c->previous = current;
            } else {
                d->first = c;
            }
            current = c;
        } while (!ts.atEnd());

        d->last = current;
        break; }

    case Sparse: {
        int index = 0;
        Chunk *current = 0;
        do {
            Chunk *chunk = new Chunk;
            chunk->from = index;
            chunk->length = qMin<int>(d->documentSize - index, d->chunkSize);
            if (!current) {
                d->first = chunk;
            } else {
                chunk->previous = current;
                current->next = chunk;
            }
            current = chunk;
            index += chunk->length;
        } while (index < d->documentSize);

        d->last = current;
        break; }
    }
//     if (d->first)
//         d->first->firstLineIndex = 0;
    Q_EMIT charactersAdded(0, d->documentSize);
    Q_EMIT documentSizeChanged(d->documentSize);
    Q_EMIT textChanged();

    return true;
}

bool TextPagerDocument::load(const QString &fileName, DeviceMode mode, QTextCodec *codec)
{
    if (mode == LoadAll) {
        QFile from(fileName);
        return from.open(QIODevice::ReadOnly) && load(&from, mode, codec);
    } else {
        QFile *file = new QFile(fileName);
        if (file->open(QIODevice::ReadOnly) && load(file, mode, codec)) {
            d->ownDevice = true;
            return true;
        } else {
            delete file;
            d->ownDevice = false;
            return false;
        }
    }
}

void TextPagerDocument::clear()
{
    setText(QString());
}

QString TextPagerDocument::read(int pos, int size) const
{
    QReadLocker locker(d->readWriteLock);
    Q_ASSERT(size >= 0);
    if (size == 0 || pos == d->documentSize) {
        return QString();
    }
    Q_ASSERT(pos < d->documentSize);

#ifndef NO_TEXTDOCUMENT_READ_CACHE
#ifdef DEBUG_CACHE_HITS
    static int hits = 0;
    static int misses = 0;
#endif
    if (d->cachePos != -1 && pos >= d->cachePos && d->cache.size() - (pos - d->cachePos) >= size) {
#ifdef DEBUG_CACHE_HITS
        qWarning() << "read hits" << ++hits << "misses" << misses;
#endif
        return d->cache.mid(pos - d->cachePos, size);
    }
#ifdef DEBUG_CACHE_HITS
    qWarning() << "read hits" << hits << "misses" << ++misses;
#endif
#endif

    QString ret(size, '\0');
    int written = 0;
    int offset;
    Chunk *c = d->chunkAt(pos, &offset);
    Q_ASSERT(c);
    int chunkPos = pos - offset;

    while (written < size && c) {
        const int max = qMin(size - written, c->size() - offset);
        const QString data = d->chunkData(c, chunkPos);
        chunkPos += data.size();
        ret.replace(written, max, data.constData() + offset, max);
        written += max;
        offset = 0;
        c = c->next;
    }

    if (written < size) {
        ret.truncate(written);
    }
    Q_ASSERT(!c || written == size);
#ifndef NO_TEXTDOCUMENT_READ_CACHE
    d->cachePos = pos;
    d->cache = ret;
#endif
    return ret;
}

QStringRef TextPagerDocument::readRef(int pos, int size) const
{
    QReadLocker locker(d->readWriteLock);
    int offset;
    Chunk *c = d->chunkAt(pos, &offset);
    if (c && pos + offset + size <= c->size()) {
        const QString string = d->chunkData(c, pos - offset);
        return string.midRef(offset, size);
    }
    return QStringRef();
}

#if 0
static bool isSameFile(const QIODevice *left, const QIODevice *right)
{
    if (left == right)
        return true;
    if (const QFile *lf = qobject_cast<const QFile *>(left)) {
        if (const QFile *rf = qobject_cast<const QFile *>(right)) {
            return QFileInfo(*lf) == QFileInfo(*rf);
        }
    }
    return false;
}
#endif

int TextPagerDocument::documentSize() const
{
    QReadLocker locker(d->readWriteLock);
    return d->documentSize;
}

int TextPagerDocument::chunkCount() const
{
    QReadLocker locker(d->readWriteLock);
    Chunk *c = d->first;
    int count = 0;
    while (c) {
        ++count;
        c = c->next;
    }
    return count;
}

int TextPagerDocument::instantiatedChunkCount() const
{
    QReadLocker locker(d->readWriteLock);
    Chunk *c = d->first;
    int count = 0;
    while (c) {
        if (!c->data.isEmpty())
            ++count;
        c = c->next;
    }
    return count;
}

int TextPagerDocument::swappedChunkCount() const
{
    QReadLocker locker(d->readWriteLock);
    Chunk *c = d->first;
    int count = 0;
    while (c) {
        if (!c->swap.isEmpty())
            ++count;
        c = c->next;
    }
    return count;
}

TextPagerDocument::DeviceMode TextPagerDocument::deviceMode() const
{
    QReadLocker locker(d->readWriteLock);
    return d->deviceMode;
}

QTextCodec * TextPagerDocument::textCodec() const
{
    QReadLocker locker(d->readWriteLock);
    return d->textCodec;
}

class FindScope
{
public:
    FindScope(TextDocumentPrivate::FindState *s) : state(s) { if (state) *state = TextDocumentPrivate::Finding; }
    ~FindScope() { if (state) *state = TextDocumentPrivate::NotFinding; }
    TextDocumentPrivate::FindState *state;
};

static void initFind(const TextPagerCursor &cursor, bool reverse, int *start, int *limit)
{
    if (cursor.hasSelection()) {
        *start = cursor.selectionStart();
        *limit = cursor.selectionEnd();
        if (reverse) {
            qSwap(*start, *limit);
        }
    } else {
        *start = cursor.position();
        *limit = (reverse ? 0 : cursor.document()->documentSize());
    }


}

static void initFindForLines(const TextPagerCursor &cursor, bool reverse, int *start, int *limit)
{
   *start = cursor.position();
   if(*limit == -1)
      	*limit = (reverse ? 0 : cursor.document()->documentSize());

#ifdef TEXTDOCUMENT_FIND_DEBUG
   qDebug() << "initial position" << "pos:" << *start << "anchor:" << cursor.anchor();
#endif

   //Change the search position according to the selection and search
   //direction
   if(cursor.anchor() >=0)
   {
   	int ca=cursor.anchor();

   	if(!reverse)
   	{
   		if(*start < ca)
   			*start=ca;
   	}
   	else
   	{
   		if(*start > ca && ca >0)
   			*start=ca-1;
   		else if(*start >0)
   			*start=(*start)-1;
   	}
   }

#ifdef TEXTDOCUMENT_FIND_DEBUG
   qDebug() << "modified position" << "pos:" << *start << "anchor:" << cursor.anchor();
#endif

}

TextPagerCursor TextPagerDocument::find(const QRegExp &regexp, const TextPagerCursor &cursor, FindMode flags,int limit) const
{
#ifdef TEXTDOCUMENT_FIND_DEBUG
	qDebug() << "---> TextPagerDocument::find" << "regexp" << regexp;
#endif

    if(documentSize() == 0)
        return TextPagerCursor();

	if(regexp.isEmpty())
		return TextPagerCursor();

	QReadLocker locker(d->readWriteLock);
    if (flags & FindWholeWords) {
        qWarning("FindWholeWords doesn't work with regexps. Instead use an actual RegExp for this");
    }
    if (flags & FindCaseSensitively) {
        qWarning("FindCaseSensitively doesn't work with regexps. Instead use an QRegExp::caseSensitivity for this");
    }
   /* if (flags & FindWrap && cursor.hasSelection()) {
        qWarning("It makes no sense to pass FindWrap and set a selection for the cursor. The entire selection will be searched");
        flags &= ~FindWrap;
    }*/

    const bool reverse = flags & FindBackward;
    int pos;
    ::initFindForLines(cursor, reverse, &pos, &limit);

    if(reverse) {
      	if(pos < limit)
        	return TextPagerCursor();
    } else {
      	if(pos > limit)
      		return TextPagerCursor();
    }

    if (pos == d->documentSize) {
        if (reverse) {
            --pos;
        } else if (!(flags & FindWrap)) {
            return TextPagerCursor();
        }
    }

    //const TextDocumentIterator::Direction direction = (reverse
    //                                                   ? TextDocumentIterator::Left
    //                                                   : TextDocumentIterator::Right);
    TextDocumentIterator it(d, pos);
    if (reverse) {
        it.setMinBoundary(limit);
    } else {
        it.setMaxBoundary(limit);
    }
    const QLatin1Char newline('\n');   
    bool ok = true;
    //int progressInterval = 0;
    const FindScope scope(flags & FindAllowInterrupt ? &d->findState : 0);
    QTime lastProgressTime;
    if (flags & FindAllowInterrupt) {
        //progressInterval = qMax<int>(1, (reverse
        //                                 ? (static_cast<qreal>(pos) / static_cast<qreal>(TEXTDOCUMENT_FIND_INTERVAL_PERCENTAGE))
        //                                 : (static_cast<qreal>(d->documentSize) - static_cast<qreal>(pos)) / 100.0));
        //maxFindLength = (reverse ? pos : d->documentSize - pos);
        lastProgressTime.start();
    }

    QString line;
    int index;
    int from;
#ifdef TEXTDOCUMENT_FIND_DEBUG
    QTime lap;
    lap.start();
#endif

    //Loop over the lines in the document. Since we can have millions of lines it has to be
    //extremely fast.
    do {
#ifdef TEXTDOCUMENT_FIND_SLEEP
        findSleep(this);
#endif

        //This clears the string without reallocation
        line.resize(0);

        if(((reverse)?it.prevLine(line):it.nextLine(line)) == 0)
        {
        	ok=false;
        	if(line.isEmpty())
        	  break;
        }

        //We only search for the first occurrence. So the FindAll flag is ignored.

        //QRegExp::lastIndexIn()  is 3 times slower than QRegExp::indexIn()!! So we always call
        //indexIn() first the lastIndexIn() if we need the reverse order.

        if((index = regexp.indexIn(line, 0)) != -1) {

#ifdef TEXTDOCUMENT_FIND_DEBUG
        	qDebug() << line;
#endif

        	if(reverse) {
        		index = regexp.lastIndexIn(line, line.size());
                from = it.position(); //-line.size();

        		if(from + index  < limit) {
        		    ok = false;
        		} else {
        			const TextPagerCursor ret(this, from + index, from + index + regexp.matchedLength());
#ifdef TEXTDOCUMENT_FIND_DEBUG
        			qDebug() << "total time" << lap.elapsed();
        			qDebug() << "result" << "pos:" << ret.position() << "anchor:" << ret.anchor();
#endif
        			return ret;
        		}
        	} else {

        		from = it.position()-line.size()+1;

        		if(from + index + regexp.matchedLength() > limit) {
        			ok = false;
        		} else {

                    //we need to remove the newline char from the end of the matching text
                    QString captured=regexp.capturedTexts().first();
                    if(index + regexp.matchedLength() == line.size() && line.endsWith(newline))
                    {
                        captured.chop(1);
                    }

                    const TextPagerCursor ret(this, from + index + captured.size(), from + index);
#ifdef TEXTDOCUMENT_FIND_DEBUG
        			qDebug() << "current:" << it.current() <<  it.position() << line.size();
                    qDebug() << "captured:" << index << captured;
        			qDebug() << "cursor:" << ret.selectedText();
#endif
                    Q_ASSERT(ret.selectedText() == captured);
#ifdef TEXTDOCUMENT_FIND_DEBUG
        			qDebug() << "total time" << lap.elapsed();
        			qDebug() << "result" << "pos:" << ret.position() << "anchor:" << ret.anchor();
#endif
        			return ret;
        		}
        	}
        }


        /*if (progressInterval != 0) {
            const int progress = qAbs(it.position() - lastProgress);
            if (progress >= progressInterval
                || (lastProgressTime.elapsed() >= TEXTDOCUMENT_MAX_INTERVAL)) {
                const qreal progress = qAbs<int>(static_cast<qreal>(it.position() - initialPos)) / static_cast<qreal>(maxFindLength);
                Q_EMIT findProgress(progress * 100.0, it.position());
                if (d->findState == TextDocumentPrivate::AbortFind) {
                    return TextPagerCursor();
                }
                lastProgress = it.position();
                lastProgressTime.restart();
            }
        }*/

    } while (ok);

#ifdef TEXTDOCUMENT_FIND_DEBUG
        qDebug() << "total time" << lap.elapsed();
        qDebug() << "iterator position:" << it.position();
#endif

    if (flags & FindWrap) {

#ifdef TEXTDOCUMENT_FIND_DEBUG
        qDebug() << "---> end of doc reached. FindWrap will start.";
#endif
    	//Q_ASSERT(!cursor.hasSelection());
        if (reverse) {
            if (cursor.position() + 1 < d->documentSize) {
                return find(regexp, TextPagerCursor(this, d->documentSize), flags & ~FindWrap, cursor.position());
            }
        } else if (cursor.position() > 0) {
            return find(regexp, TextPagerCursor(this, 0), flags & ~FindWrap, cursor.position());
        }
    }

    return TextPagerCursor();
}

TextPagerCursor TextPagerDocument::find(const QString &in, const TextPagerCursor &cursor, FindMode flags, int limit) const
{
#ifdef TEXTDOCUMENT_FIND_DEBUG
	qDebug() << "---> TextPagerDocument::find" << "string:" << in;
#endif

	if (in.isEmpty()) {
        return TextPagerCursor();
    }

    QReadLocker locker(d->readWriteLock);

    const bool reverse = flags & FindBackward;
    const bool caseSensitive = flags & FindCaseSensitively;
    const bool wholeWords = flags & FindWholeWords;

    /* if (flags & FindWrap && cursor.hasSelection()) {
        qWarning("It makes no sense to pass FindWrap and set a selection for the cursor. The entire selection will be searched");
        flags &= ~FindWrap;
    }
*/
    const QLatin1Char newline('\n');
    int pos;
    ::initFindForLines(cursor, reverse, &pos, &limit);

    //Sanity check!!
    Q_ASSERT(pos >= 0 && pos <= d->documentSize);
    Q_ASSERT(limit >= 0 && limit <= d->documentSize);

    if(reverse) {
    	if(pos < limit)
    		return TextPagerCursor();
    } else {
    	if(pos > limit)
    		return TextPagerCursor();
    }

    if (pos == d->documentSize) {
        if (reverse) {
            --pos;
        } else if (!(flags & FindWrap)) {
            return TextPagerCursor();
        }
    }

    // ### what if one searches for a string with non-word characters in it and FindWholeWords?
    //const TextDocumentIterator::Direction direction = (reverse ? TextDocumentIterator::Left : TextDocumentIterator::Right);
    QString word = caseSensitive ? in : in.toLower();

    TextDocumentIterator it(d, pos);
    if (reverse) {
        it.setMinBoundary(limit);
    } else {
        it.setMaxBoundary(limit);
    }

#ifdef TEXTDOCUMENT_FIND_DEBUG
    qDebug() << "current character:" << it.current();
#endif

    if (!caseSensitive)
        it.setConvertToLowerCase(true);

    bool ok = true;
    //QChar ch = it.current();
    //int progressInterval = 0;
    const FindScope scope(flags & FindAllowInterrupt ? &d->findState : 0);
    QTime lastProgressTime;
    if (flags & FindAllowInterrupt) {
        //progressInterval = qMax<int>(1, (reverse
        //                                 ? (static_cast<qreal>(pos) / static_cast<qreal>(TEXTDOCUMENT_FIND_INTERVAL_PERCENTAGE))
        //                                 : (static_cast<qreal>(d->documentSize) - static_cast<qreal>(pos)) / 100.0));
        //maxFindLength = (reverse ? pos : d->documentSize - pos);
        lastProgressTime.start();
    }

    QString line;
    int index;
    int from;
#ifdef TEXTDOCUMENT_FIND_DEBUG
    QTime lap;
    lap.start();
#endif

    do {
#ifdef TEXTDOCUMENT_FIND_SLEEP
        findSleep(this);
#endif
     /*   if (progressInterval != 0) {
            const int progress = qAbs(it.position() - lastProgress);
            if (progress >= progressInterval
                || (progress % 10 == 0 && lastProgressTime.elapsed() >= TEXTDOCUMENT_MAX_INTERVAL)) {
                const qreal progress = qAbs<int>(static_cast<qreal>(it.position() - initialPos)) / static_cast<qreal>(maxFindLength);
                Q_EMIT findProgress(progress * 100.0, it.position());
                if (d->findState == TextDocumentPrivate::AbortFind) {
                    return TextPagerCursor();
                }
                lastProgress = it.position();
                lastProgressTime.restart();
            }
        }*/

        //This clears the string without reallocation
        line.resize(0);

        if(((reverse)?it.prevLine(line):it.nextLine(line)) == 0)
        {
            ok=false;
            if(line.isEmpty())
            	break;
        }

        if(!caseSensitive)
        	line=line.toLower();

#ifdef TEXTDOCUMENT_FIND_DEBUG
        //qDebug() << line;
#endif

        if((index=line.indexOf(word)) != -1) {

            //Backward:
        	//The iterator is positioned at the linebreak character of the previous line, or at
        	//the start of the document
        	if(reverse) {
        		from = it.position();
        		index=line.lastIndexOf(word);
        	}
        	//Forward:
        	//The iterator is positioned at the linebreak character at the end of the line or at
        	//the end of the document
        	else {
                from = it.position()-line.size()+1;
        	}

        	while(ok && index != -1) {

        		const int startPos=from + index;
        		const int endPos=from + index + word.size();

        		if(!reverse && endPos > limit) {
        			ok = false;
        			break;
        		}

        		bool found=true;
        		if(wholeWords) {

        			if(TextDocumentIterator::Left != d->wordBoundariesAt(startPos)) {
        				found = false;
        			}
        			if(found) {
        				if(TextDocumentIterator::Right != d->wordBoundariesAt(endPos-1)) {
        					found = false;
        				}
        			}
        		}

        		if(found) {

#ifdef TEXTDOCUMENT_FIND_DEBUG
        			qDebug() << line;
#endif
        			//Backward
        			if(reverse) {
        				TextPagerCursor ret(this, startPos, endPos);
#ifdef TEXTDOCUMENT_FIND_DEBUG
        				qDebug() << "total time" << lap.elapsed();
        				qDebug() << "current:" << it.current() <<  it.position() << line.size();
        				qDebug() << "result" << "pos:" << ret.position() << "anchor:" << ret.anchor();
#endif
        				return ret;
        			//Forward
        			} else {

        				TextPagerCursor ret(this, endPos,startPos);
#ifdef TEXTDOCUMENT_FIND_DEBUG
        				qDebug() << "total time" << lap.elapsed();
        				qDebug() << "current:" << it.current() <<  it.position() << line.size();
        				qDebug() << "result" << "pos:" << ret.position() << "anchor:" << ret.anchor();
#endif
        				return ret;
        			}

        		//If it is not a wholeword we try to match every other match int he same line
        		} else {

        			if(reverse) {
        			      index=line.lastIndexOf(word,index-1);
        			} else {
        				index=line.indexOf(word,index+word.size());
        			}

        		}
        	} //while

        }

#if 0
        bool found = ch == word.at(wordIndex);
        if (found && wholeWords && (wordIndex == 0 || wordIndex == word.size() - 1)) {
            Q_ASSERT(word.size() > 1);
            const uint requiredBounds = ((wordIndex == 0) != reverse)
                                        ? TextDocumentIterator::Left
                                        : TextDocumentIterator::Right;
            const uint bounds = d->wordBoundariesAt(it.position());
            if (requiredBounds & ~bounds) {
                found = false;
            }
        }
        if (found) {
            if (++wordIndex == word.size()) {
                const int pos = it.position() - (reverse ? 0 : word.size() - 1);
                // the iterator reads one past the last matched character so we have to account for that here
                const TextPagerCursor ret(this, pos + wordIndex, pos);
                if (flags & FindAll) {
                    Q_EMIT entryFound(ret);
                    if (d->findState == TextDocumentPrivate::AbortFind)
                        return TextPagerCursor();
                    wordIndex = 0;
                } else {
                    return ret;
                }
            }
        } else if (wordIndex != 0) {
            wordIndex = 0;
            continue;
        }
        ch = it.nextPrev(direction, ok);

#endif

    } while (ok);

#ifdef TEXTDOCUMENT_FIND_DEBUG
      qDebug() << "total time" << lap.elapsed();
#endif

    if (flags & FindWrap) {
        //Q_ASSERT(!cursor.hasSelection());
        if (reverse) {

        	if (cursor.position() + 1 < d->documentSize) {
        		  return find(in, TextPagerCursor(this, d->documentSize), flags & ~FindWrap,cursor.position());
            }
        } else if (cursor.position() > 0) {
        	return find(in, TextPagerCursor(this, 0), flags & ~FindWrap,cursor.position());
        }
    }

    return TextPagerCursor();
}

TextPagerCursor TextPagerDocument::find(const QChar &chIn, const TextPagerCursor &cursor, FindMode flags) const
{
    QReadLocker locker(d->readWriteLock);
    if (flags & FindWrap && cursor.hasSelection()) {
        qWarning("It makes no sense to pass FindWrap and set a selection for the cursor. The entire selection will be searched");
        flags &= ~FindWrap;
    }

    const bool reverse = flags & FindBackward;
    int pos;
    int limit;
    ::initFind(cursor, reverse, &pos, &limit);
    if (pos == d->documentSize) {
        if (reverse) {
            --pos;
        } else if (!(flags & FindWrap)) {
            return TextPagerCursor();
        }
    }
    Q_ASSERT(pos >= 0 && pos <= d->documentSize);

    const bool caseSensitive = flags & FindCaseSensitively;
    const bool wholeWords = flags & FindWholeWords;
    const QChar ch = (caseSensitive ? chIn : chIn.toLower());
    TextDocumentIterator it(d, pos);
    if (reverse) {
        it.setMinBoundary(limit);
    } else {
        it.setMaxBoundary(limit);
    }
    const TextDocumentIterator::Direction dir = (reverse
                                                 ? TextDocumentIterator::Left
                                                 : TextDocumentIterator::Right);
    int lastProgress = pos;
    const int initialPos = pos;
    int maxFindLength = 0;
    int progressInterval = 0;
    const FindScope scope(flags & FindAllowInterrupt ? &d->findState : 0);
    QTime lastProgressTime;
    if (flags & FindAllowInterrupt) {
        progressInterval = qMax<int>(1, (reverse
                                         ? (static_cast<qreal>(pos) / static_cast<qreal>(TEXTDOCUMENT_FIND_INTERVAL_PERCENTAGE))
                                         : (static_cast<qreal>(d->documentSize) - static_cast<qreal>(pos)) / 100.0));
        maxFindLength = (reverse ? pos : d->documentSize - pos);
        lastProgressTime.start();
    }

    QChar c = it.current();
    bool ok = true;
    do {
#ifdef TEXTDOCUMENT_FIND_SLEEP
        findSleep(this);
#endif
        if (((caseSensitive ? c : c.toLower()) == ch)
            && (!wholeWords || (d->wordBoundariesAt(it.position()) == TextDocumentIterator::Both))) {
            const TextPagerCursor ret(this, it.position() + 1, it.position());
            if (flags & FindAll) {
                Q_EMIT entryFound(ret);
                if (d->findState == TextDocumentPrivate::AbortFind)
                    return TextPagerCursor();
            } else {
                return ret;
            }
        }
        c = it.nextPrev(dir, ok);
//         qDebug() << "progressInterval" << progressInterval << qAbs(it.position() - lastProgress)
//                  << lastProgressTime.elapsed() << TEXTDOCUMENT_MAX_INTERVAL;
        if (progressInterval != 0) {
            const int progress = qAbs(it.position() - lastProgress);
            if (progress >= progressInterval
                || (progress % 10 == 0 && lastProgressTime.elapsed() >= TEXTDOCUMENT_MAX_INTERVAL)) {
                const qreal progress = qAbs<int>(static_cast<qreal>(it.position() - initialPos)) / static_cast<qreal>(maxFindLength);
                Q_EMIT findProgress(progress * 100.0, it.position());
                if (d->findState == TextDocumentPrivate::AbortFind) {
                    return TextPagerCursor();
                }
                lastProgress = it.position();
                lastProgressTime.restart();
            }
        }
    } while (ok);

    if (flags & FindWrap) {
        Q_ASSERT(!cursor.hasSelection());
        if (reverse) {
            if (cursor.position() + 1 < d->documentSize) {
                return find(ch, TextPagerCursor(this, cursor.position(), d->documentSize), flags & ~FindWrap);
            }
        } else if (cursor.position() > 0) {
            return find(ch, TextPagerCursor(this, 0, cursor.position()), flags & ~FindWrap);
        }
    }

    return TextPagerCursor();
}

static inline int count(const QString &string, int from, int size, const QChar &ch)
{
    Q_ASSERT(from + size <= string.size());
    const ushort needle = ch.unicode();
    const ushort *haystack = string.utf16() + from;
    int num = 0;
    for (int i=0; i<size; ++i) {
        if (*haystack++ == needle)
            ++num;
    }
//    Q_ASSERT(string.mid(from, size).count(ch) == num);
    return num;
}

void TextPagerDocument::takeTextSection(TextPagerSection *section)
{
    QWriteLocker locker(d->readWriteLock);
    Q_ASSERT(section);
    Q_ASSERT(section->document() == this);

    QList<TextPagerSection*>::iterator first = qLowerBound(d->sections.begin(), d->sections.end(), section, compareTextSection);
    Q_ASSERT(first != d->sections.end());
    const QList<TextPagerSection*>::iterator last = qUpperBound(d->sections.begin(), d->sections.end(), section, compareTextSection);

    while (first != last) {
        if (*first == section) {
            Q_EMIT sectionRemoved(section);
            d->sections.erase(first);
            break;
        }
        ++first;
    }

    // Moved this to the end as the slots called by sectionRemoved (presently) rely
    // on section->d.document to be valid.
    section->d.textEdit = 0;
    section->d.document = 0;
}

QList<TextPagerSection*> TextPagerDocument::sections(int pos, int size, TextPagerSection::TextSectionOptions flags) const
{
    QReadLocker locker(d->readWriteLock);
    return d->getSections(pos, size, flags, 0);
}

void TextPagerDocument::insertTextSection(TextPagerSection *section)
{
    QWriteLocker locker(d->readWriteLock);
    Q_ASSERT(!d->sections.contains(section));
    QList<TextPagerSection*>::iterator it = qLowerBound<QList<TextPagerSection*>::iterator>(d->sections.begin(), d->sections.end(),
                                                                                  section, compareTextSection);
    d->sections.insert(it, section);
    Q_EMIT sectionAdded(section);
}

TextPagerSection *TextPagerDocument::insertTextSection(int pos, int size,
                                             const QTextCharFormat &format, const QVariant &data)
{
    QWriteLocker locker(d->readWriteLock);
    Q_ASSERT(pos >= 0);
    Q_ASSERT(size >= 0);
    Q_ASSERT(pos < d->documentSize);

    TextPagerSection *l = new TextPagerSection(pos, size, this, format, data);
    QList<TextPagerSection*>::iterator it = qLowerBound<QList<TextPagerSection*>::iterator>(d->sections.begin(), d->sections.end(), l, compareTextSection);
    d->sections.insert(it, l);
    Q_EMIT sectionAdded(l);
    return l;
}

bool TextPagerDocument::abortSave()
{
    if (d->saveState == TextDocumentPrivate::Saving) {
        d->saveState = TextDocumentPrivate::AbortSave;
        return true;
    }
    return false;
}

bool TextPagerDocument::abortFind() const
{
    if (d->findState == TextDocumentPrivate::Finding) {
        d->findState = TextDocumentPrivate::AbortFind;
        return true;
    }
    return false;
}


QChar TextPagerDocument::readCharacter(int pos) const
{
    QReadLocker locker(d->readWriteLock);
    if (pos == d->documentSize)
        return QChar();
    Q_ASSERT(pos >= 0 && pos < d->documentSize);
#ifndef NO_TEXTDOCUMENT_READ_CACHE
#ifdef DEBUG_CACHE_HITS
    static int hits = 0;
    static int misses = 0;
#endif
    if (pos >= d->cachePos && pos < d->cachePos + d->cache.size()) {
#ifdef DEBUG_CACHE_HITS
        qWarning() << "readCharacter hits" << ++hits << "misses" << misses;
#endif
        return d->cache.at(pos - d->cachePos);
    }
#ifdef DEBUG_CACHE_HITS
    qWarning() << "readCharacter hits" << hits << "misses" << ++misses;
#endif
#endif

    int offset;
    Chunk *c = d->chunkAt(pos, &offset);
    return d->chunkData(c, pos - offset).at(offset);
}

void TextPagerDocument::setText(const QString &text)
{
    // ### could optimize this to avoid detach and copy if text.size() <= chunkSize
    // ### but is it worth it?
    QBuffer buffer;
    buffer.open(QIODevice::WriteOnly);
    QTextStream ts(&buffer);
    if (d->textCodec)
        ts.setCodec(d->textCodec);
    ts << text;
    buffer.close();
    buffer.open(QIODevice::ReadOnly);
    const bool ret = load(&buffer, LoadAll);
    Q_ASSERT(ret);
    Q_UNUSED(ret);
}

int TextPagerDocument::chunkSize() const
{
    QReadLocker locker(d->readWriteLock);
    return d->chunkSize;
}

void TextPagerDocument::setChunkSize(int size)
{
    QWriteLocker locker(d->readWriteLock);
    Q_ASSERT(d->chunkSize > 0);
    d->chunkSize = size;
}

int TextPagerDocument::currentMemoryUsage() const
{
    QReadLocker locker(d->readWriteLock);
    Chunk *c = d->first;
    int used = 0;
    while (c) {
        used += c->data.size() * sizeof(QChar);
        c = c->next;
    }
    return used;
}

bool TextPagerDocument::isModified() const
{
    // ### should it lock for read?
    return d->modified;
}

int TextPagerDocument::lineNumber(int position) const
{
    QReadLocker locker(d->readWriteLock);
    d->hasChunksWithLineNumbers = true; // does this need to be a write lock?
    int offset;
    Chunk *c = d->chunkAt(position, &offset);
    d->updateChunkLineNumbers(c, position - offset);
    Q_ASSERT(c->firstLineIndex != -1);
    Q_ASSERT(d->first->firstLineIndex != -1);
    const int extra = (offset == 0 ? 0 : d->countNewLines(c, position - offset, offset));

//#ifdef QT_DEBUG
#if 0
    if (position <= 16000) {
        const QString data = read(0, position);
        // if we're on a newline it shouldn't count so we do read(0, position)
        // not read(0, position + 1);
        const int count = data.count(QLatin1Char('\n'));
        if (count != c->firstLineIndex + extra) {
            qDebug() << "TextPagerDocument::lineNumber returns" << (c->firstLineIndex + extra)
                     << "should have returned" << (count + 1)
                     << "for index" << position;
        }
    }
#endif

    return c->firstLineIndex + extra;
}

int TextPagerDocument::columnNumber(int position) const
{
    TextPagerCursor cursor(this, position);
    return cursor.isNull() ? -1 : cursor.columnNumber();
}

int TextPagerDocument::lineNumber(const TextPagerCursor &cursor) const
{
    return cursor.document() == this ? lineNumber(cursor.position()) : -1;
}

int TextPagerDocument::columnNumber(const TextPagerCursor &cursor) const
{
    return cursor.document() == this ? cursor.columnNumber() : -1;
}

TextPagerCursor TextPagerDocument::findLine(int lineNum, const TextPagerCursor &cursor) const
{
    if(lineNum <=0)
        return TextPagerCursor();
    
    lineNum--;

    Q_ASSERT(cursor.position() >=0);
    
    int current=lineNumber(cursor);

#ifdef TEXTDOCUMENT_FIND_DEBUG            
    qDebug() << "findLine --> line:" << lineNum << "current:" << current; 
#endif        
        
    if(lineNum == current)
        return cursor;
   
    int offset;
    Chunk *c = d->chunkAt(cursor.position(), &offset);
    
    Q_ASSERT(c != NULL);
    
    int pos=cursor.position() - offset;  //points to the chunks beginning
    d->updateChunkLineNumbers(c, pos);
  
#ifdef TEXTDOCUMENT_FIND_DEBUG            
    qDebug() << "chunk - first line:" << c->firstLineIndex << "pos:" << pos;
#endif          
    
    if(lineNum < current) {       
            
        while(c->firstLineIndex > lineNum && c->previous) {
            pos-=c->size();
            c=c->previous;
            d->updateChunkLineNumbers(c,pos);
#ifdef TEXTDOCUMENT_FIND_DEBUG 
            //qDebug() << "chunk - first line:" << c->firstLineIndex << "pos:" << pos;
#endif
        }  
        
    } else if(lineNum > current) {

        while(c->firstLineIndex < lineNum && c->next) {
            pos+=c->size();
            c=c->next;
            d->updateChunkLineNumbers(c,pos);
#ifdef TEXTDOCUMENT_FIND_DEBUG 
            //qDebug() << "chunk - first line:" << c->firstLineIndex << "pos:" << pos;
#endif           
        }  
        
        Q_ASSERT(c != NULL && c->previous != NULL);
        
        c=c->previous;
        pos-=c->size();
    }
       
#ifdef TEXTDOCUMENT_FIND_DEBUG  
    if(c) qDebug() << "chunk found - first line:" << c->firstLineIndex << "pos:" << pos;
    else qDebug() << "chunk not found";
#endif   
    if(c && c->firstLineIndex != -1 && c->firstLineIndex <= lineNum) {
            
        current=c->firstLineIndex;
        if(current == lineNum)
            return TextPagerCursor(this,pos);
            
        QChar newline('\n');         
        int index=0;
        QString data=d->chunkData(c,-1);
        while((index=data.indexOf(newline,index)) != -1) {
#ifdef TEXTDOCUMENT_FIND_DEBUG  
            //qDebug() << "chunk found - line:" << current << "index:" << index;
#endif       
            if(current == lineNum) {
                TextPagerCursor ret(this,pos+index);
                    return ret;
            }
            index++;
            current++; 
        }
    }        
        
    return TextPagerCursor();
}


void TextPagerDocument::setOptions(Options opt)
{
    d->options = opt;
    if ((d->options & Locking) != (d->readWriteLock != 0)) {
        if (d->readWriteLock) {
            delete d->readWriteLock;
        } else {
            d->readWriteLock = new QReadWriteLock(QReadWriteLock::Recursive);
        }
    }
}

TextPagerDocument::Options TextPagerDocument::options() const
{
    return d->options;
}

bool TextPagerDocument::isWordCharacter(const QChar &ch, int /*index*/) const
{
    // from qregexp.
    return ch.isLetterOrNumber() || ch.isMark() || ch == QLatin1Char('_');
}

QString TextPagerDocument::swapFileName(Chunk *chunk)
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    QReadLocker locker(d->readWriteLock);
    QString file = QStandardPaths::standardLocations(QStandardPaths::TempLocation).first();
    file.reserve(file.size() + 24);
    QTextStream ts(&file);
    ts << QLatin1Char('/') << QLatin1String("lte_") << chunk << QLatin1Char('_')
       << this << QLatin1Char('_') << QCoreApplication::applicationPid();
    return file;
#endif
    return QString();
}

//===========================================================================
//
//   TextDocumentPrivate
//
//===========================================================================

Chunk *TextDocumentPrivate::chunkAt(int p, int *offset) const
{
    Q_ASSERT(p <= documentSize);
    Q_ASSERT(p >= 0);
    Q_ASSERT(first);
    Q_ASSERT(last);
    if (p == documentSize) {
        if (offset)
            *offset = last->size();
        return last;
    }
#ifndef NO_TEXTDOCUMENT_CHUNK_CACHE
    Q_ASSERT(!cachedChunk || cachedChunkPos != -1);
    if (cachedChunk && p >= cachedChunkPos && p < cachedChunkPos + cachedChunkData.size()) {
        if (offset)
            *offset = p - cachedChunkPos;
        return cachedChunk;
    }
#endif
    int pos = p;
    Chunk *c = first;

    Q_FOREVER {
        const int size = c->size();
        if (pos < size) {
            break;
        }
        pos -= size;
        c = c->next;
        Q_ASSERT(c);
    }

    if (offset)
        *offset = pos;

    Q_ASSERT(c);
    return c;
}


/* Evil double meaning of pos here. If it's -1 we don't cache it. */
QString TextDocumentPrivate::chunkData(const Chunk *chunk, int chunkPos) const
{
#ifndef NO_TEXTDOCUMENT_CHUNK_CACHE
#ifdef DEBUG_CACHE_HITS
    static int hits = 0;
    static int misses = 0;
#endif
    if (chunk == cachedChunk) {
#ifdef DEBUG_CACHE_HITS
        qWarning() << "chunkData hits" << ++hits << "misses" << misses;
#endif
        Q_ASSERT(cachedChunkData.size() == chunk->size());
        return cachedChunkData;
    } else
#endif
    if (chunk->from == -1) {
        return chunk->data;
    } else if (!device && chunk->swap.isEmpty()) {
        // Can only happen if the device gets deleted behind our back when in Sparse mode
        return QString().fill(QLatin1Char(' '), chunk->size());
    } else {
        QIODevice *dev = device.data();
#if 0
        QFile file;
        if (!chunk->swap.isEmpty()) {
            file.setFileName(chunk->swap);
            if (!file.open(QIODevice::ReadOnly)) {
                qWarning("TextDocumentPrivate::chunkData() Can't open file for reading '%s'", qPrintable(chunk->swap));
                return QString().fill(QLatin1Char(' '), chunk->size());
            }
            dev = &file;
        }
#endif
        QTextStream ts(dev);
        //if (textCodec)
         //   ts.setCodec(textCodec);
//         if (!chunk->swap.isEmpty()) {
//             qDebug() << "reading stuff from swap" << chunk << chunk->from << chunk->size() << chunk->swap;
//         }
       ts.seek(chunk->from);

#ifndef NO_TEXTDOCUMENT_CHUNK_CACHE

        if (chunkPos != -1) {
        	cachedChunkData =  ts.read(chunk->length);

        	Q_ASSERT(cachedChunkData.size() == chunk->size());

        	cachedChunk = const_cast<Chunk*>(chunk);
        	cachedChunkPos = chunkPos;

        	return cachedChunkData;
/*#ifdef QT_DEBUG
        	if (chunkPos != chunk->pos()) {
        	     qWarning() << chunkPos << chunk->pos();
        	 }
        	 Q_ASSERT(chunkPos == chunk->pos());
#endif*/
        } else {
        	const QString data = ts.read(chunk->length);
        	return data;
        }

#else
        const QString data = ts.read(chunk->length);
        return data;
#endif
    }
        //Q_ASSERT(data.size() == chunk->size());
#ifndef NO_TEXTDOCUMENT_CHUNK_CACHE
#ifdef DEBUG_CACHE_HITS
        qWarning() << "chunkData hits" << hits << "misses" << ++misses;
#endif
#endif

#if 0
        if (chunkPos != -1) {
            cachedChunk = const_cast<Chunk*>(chunk);
            cachedChunkData = data;
            cachedChunkPos = chunkPos;
#ifdef QT_DEBUG
            if (chunkPos != chunk->pos()) {
                qWarning() << chunkPos << chunk->pos();
            }
            Q_ASSERT(chunkPos == chunk->pos());
#endif
        }

        //return data;
    }
#endif
	return QString();
}

int TextDocumentPrivate::chunkIndex(const Chunk *c) const
{
    int index = 0;
    while (c->previous) {
        ++index;
        c = c->previous;
    }
    return index;
}

void TextDocumentPrivate::instantiateChunk(Chunk *chunk)
{
    if (chunk->from == -1 && chunk->swap.isEmpty())
        return;
    chunk->data = chunkData(chunk, -1);
//    qDebug() << "instantiateChunk" << chunk << chunk->swap;
    chunk->swap.clear();
#ifndef NO_TEXTDOCUMENT_CHUNK_CACHE
    // Don't want to cache this chunk since it's going away. If it
    // already was cached then sure, but otherwise don't
    if (chunk == cachedChunk) {
        cachedChunk = 0;
        cachedChunkPos = -1;
        cachedChunkData.clear();
    }
#endif
    chunk->from = chunk->length = -1;
}

void TextDocumentPrivate::removeChunk(Chunk *c)
{
    Q_ASSERT(c);
    if (c == first) {
        first = c->next;
    } else {
        c->previous->next = c->next;
    }
    if (c == last) {
        last = c->previous;
    } else {
        c->next->previous = c->previous;
    }
#ifndef NO_TEXTDOCUMENT_CHUNK_CACHE
    if (c == cachedChunk) {
        cachedChunk = 0;
        cachedChunkPos = -1;
        cachedChunkData.clear();
    }
#endif
    if (!first) {
        Q_ASSERT(!last);
        first = last = new Chunk;
    }

    delete c;
}

QString TextDocumentPrivate::wordAt(int position, int *start) const
{
    TextDocumentIterator from(this, position);
    if (!q->isWordCharacter(from.current(), position)) {
        if (start)
            *start = -1;
        return QString();
    }

    while (from.hasPrevious()) {
        const QChar ch = from.previous();
        if (!q->isWordCharacter(ch, from.position())) {
            // ### could just peek rather than going one too far
            from.next();
            break;
        }
    }
    TextDocumentIterator to(this, position);
    while (to.hasNext()) {
        const QChar ch = to.next();
        if (!q->isWordCharacter(ch, to.position()))
            break;
    }

    if (start)
        *start = from.position();
    return q->read(from.position(), to.position() - from.position());
}

QString TextDocumentPrivate::paragraphAt(int position, int *start) const
{
    const QLatin1Char newline('\n');
    TextDocumentIterator from(this, position);
    while (from.hasPrevious() && from.previous() != newline)
        ;
    TextDocumentIterator to(this, position);
    while (to.hasNext() && to.next() != newline)
        ;
    if (start)
        *start = from.position();
    return q->read(from.position(), to.position() - from.position());
}

uint TextDocumentPrivate::wordBoundariesAt(int pos) const
{
    Q_ASSERT(pos >= 0 && pos < documentSize);
    uint ret = 0;
    if (pos == 0 || !q->isWordCharacter(q->readCharacter(pos - 1), pos - 1)) {
        ret |= TextDocumentIterator::Left;
    }
    if (pos + 1 == documentSize || !q->isWordCharacter(q->readCharacter(pos + 1), pos + 1)) {
        ret |= TextDocumentIterator::Right;
    }
    return ret;
}

void TextDocumentPrivate::updateChunkLineNumbers(Chunk *c, int chunkPos) const
{
    Q_ASSERT(c);
    if (c->firstLineIndex == -1) {
        Chunk *cc = c;
        int pos = chunkPos;
        while (cc->previous && cc->previous->firstLineIndex == -1) {
            //pos -= cc->size();
        	//Here chunkPos points to the position (the beginning) of the chunk so
        	//the line above was incorrect had to be changed like this:
            pos -=cc->previous->size();
            cc = cc->previous;
        }
        // cc is at the first chunk that has firstLineIndex != -1
        Q_ASSERT(!cc->previous || cc->previous->firstLineIndex != -1);
        Q_ASSERT(cc->firstLineIndex == 1 || cc->firstLineIndex == -1);
        // special case for first chunk
        do {
            const int size = cc->size();
            if (!cc->previous) {
                cc->firstLineIndex = 0;
            } else {
                const int prevSize = cc->previous->size();
                const int lineCount = countNewLines(cc->previous, pos - prevSize, prevSize);
                Q_ASSERT(cc->previous->firstLineIndex != -1);
                cc->firstLineIndex = cc->previous->firstLineIndex + lineCount;
            }
            pos += size;
            cc = cc->next;
        } while (cc && cc != c->next);
        countNewLines(c, chunkPos, c->size());
    }
    Q_ASSERT(c->firstLineIndex != -1);
}

static inline QList<int> dumpNewLines(const QString &string, int from, int size)
{
    QList<int> ret;
    for (int i=from; i<from + size; ++i) {
        if (string.at(i) == QLatin1Char('\n'))
            ret.append(i);
    }
    return ret;
}


int TextDocumentPrivate::countNewLines(Chunk *c, int chunkPos, int size) const
{
//     qDebug() << "CALLING countNewLines on" << chunkIndex(c) << chunkPos << size;
//     qDebug() << (c == first) << c->firstLineIndex << chunkPos << size
//              << c->size();
    int ret = 0;
#ifndef TEXTDOCUMENT_LINENUMBER_CACHE
    if (size == c->size()) {
        if (c->lines == -1) {
            c->lines = ::count(chunkData(c, chunkPos), 0, size, QLatin1Char('\n'));
//             qDebug() << "counting" << c->lines << "in" << chunkIndex(c)
//                      << "Size" << size << "chunkPos" << chunkPos;
        }
        ret = c->lines;
    } else {
        ret = ::count(chunkData(c, chunkPos), 0, size, QLatin1Char('\n'));
    }
#else
//     qDebug() << size << ret << c->lineNumbers << chunkPos
//              << dumpNewLines(chunkData(c, chunkPos), 0, c->size());
    static const int lineNumberCacheInterval = TEXTDOCUMENT_LINENUMBER_CACHE_INTERVAL;
    if (c->lineNumbers.isEmpty()) {
        const QString data = chunkData(c, chunkPos);
        const int s = data.size();
        c->lineNumbers.fill(0, (s + lineNumberCacheInterval - 1) / lineNumberCacheInterval);
//        qDebug() << data.size() << c->lineNumbers.size() << lineNumberCacheInterval;

        for (int i=0; i<s; ++i) {
            if (data.at(i) == QLatin1Char('\n')) {
                ++c->lineNumbers[i / lineNumberCacheInterval];
//                 qDebug() << "found one at" << i << "put it in" << (i / lineNumberCacheInterval)
//                          << "chunkPos" << chunkPos;
                if (i < size)
                    ++ret;
            }
        }
    } else {
        for (int i=0; i<c->lineNumbers.size(); ++i) {
            if (i * lineNumberCacheInterval > size) {
                break;
            } else if (c->lineNumbers.at(i) == 0) {
                // nothing in this area
                continue;
            } else if ((i + 1) * lineNumberCacheInterval > size) {
                ret += ::count(chunkData(c, chunkPos), i * lineNumberCacheInterval,
                               size - i * lineNumberCacheInterval, QChar('\n'));
                // partly
                break;
            } else {
                ret += c->lineNumbers.at(i);
            }
        }
    }
#endif
    return ret;
}

void TextDocumentPrivate::swapOutChunk(Chunk *c)
{
    if (!c->swap.isEmpty())
        return;
    Q_ASSERT(!c->data.isEmpty());
    c->from = 0;
    c->length = c->data.size();
    c->swap = q->swapFileName(c);
    QFile file(c->swap);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning("TextDocumentPrivate::chunkData() Can't open file for writing '%s'", qPrintable(c->swap));
        c->swap.clear();
        return;
    }
    QTextStream ts(&file);
    if (textCodec)
        ts.setCodec(textCodec);
    ts << c->data;
    c->data.clear();
#ifndef NO_TEXTDOCUMENT_CHUNK_CACHE
    // ### do I want to do this?
    if (cachedChunk == c) {
        cachedChunk = 0;
        cachedChunkPos = -1;
        cachedChunkData.clear();
    }
#endif
}

static inline bool match(int pos, int left, int size)
{
    return pos >= left && pos < left + size;
}

static inline bool match(int pos, int size, const TextPagerSection *section, TextPagerSection::TextSectionOptions flags)
{
    const int sectionPos = section->position();
    const int sectionSize = section->size();

    if (::match(sectionPos, pos, size) && ::match(sectionPos + sectionSize - 1, pos, size)) {
        return true;
    } else if (flags & TextPagerSection::IncludePartial) {
        const int boundaries[] = { pos, pos + size - 1 };
        for (int boundarie : boundaries) {
            if (::match(boundarie, sectionPos, sectionSize))
                return true;
        }
    }
    return false;
}

static inline void filter(QList<TextPagerSection*> &sections, const TextPagerEdit *filter)
{
    if (filter) {
        for (int i=sections.size() - 1; i>=0; --i) {
            if (!::matchSection(sections.at(i), filter))
                sections.removeAt(i);
        }
    }
}

QList<TextPagerSection*> TextDocumentPrivate::getSections(int pos, int size, TextPagerSection::TextSectionOptions flags, const TextPagerEdit *filter) const
{
    if (size == -1)
        size = documentSize - pos;
    QList<TextPagerSection*> ret;
    if (pos == 0 && size == documentSize) {
        ret = sections;
        ::filter(ret, filter);
        return ret;
    }
    // binary search. TextPagerSections are sorted in order of position
    if (sections.isEmpty()) {
        return ret;
    }

    const TextPagerSection tmp(pos, size, static_cast<TextPagerDocument*>(0), QTextCharFormat(), QVariant());
    QList<TextPagerSection*>::const_iterator it = qLowerBound(sections.begin(), sections.end(), &tmp, compareTextSection);
    if (flags & TextPagerSection::IncludePartial && it != sections.begin()) {
        QList<TextPagerSection*>::const_iterator prev = it;
        do {
            if (::match(pos, size, *--prev, flags))
                ret.append(*prev);
        } while (prev != sections.begin());
    }
    while (it != sections.end()) {
        if (::match(pos, size, *it, flags)) {
            ret.append(*it);
        } else {
            break;
        }
        ++it;
    }
    ::filter(ret, filter);
    return ret;
}

void TextDocumentPrivate::textEditDestroyed(TextPagerEdit *edit)
{
    QMutableListIterator<TextPagerSection*> i(sections);
    while (i.hasNext()) {
        TextPagerSection *section = i.next();
        if (section->textEdit() == edit) {
            section->d.document = 0;
            // Make sure we also remove it from the list of sections so it
            // isn't deleted in the TextDocument destructor too.
            i.remove();
            delete section;
        }
    }
}

void TextPagerDocument::lockForRead()
{
    Q_ASSERT(d->readWriteLock);
    d->readWriteLock->lockForRead();
}

void TextPagerDocument::lockForWrite()
{
    Q_ASSERT(d->readWriteLock);
    d->readWriteLock->lockForWrite();
}

bool TextPagerDocument::tryLockForRead()
{
    Q_ASSERT(d->readWriteLock);
    return d->readWriteLock->tryLockForRead();
}

bool TextPagerDocument::tryLockForWrite()
{
    Q_ASSERT(d->readWriteLock);
    return d->readWriteLock->tryLockForWrite();
}

void TextPagerDocument::unlock()
{
    Q_ASSERT(d->readWriteLock);
    d->readWriteLock->unlock();
}

