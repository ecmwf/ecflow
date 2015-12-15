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

#ifndef TEXTPAGEREDIT_HPP__
#define TEXTPAGEREDIT_HPP__

#include <QtGui>
#include <QAbstractScrollArea>

#include "syntaxhighlighter.hpp"
#include "TextPagerDocument.hpp"
#include "TextPagerCursor.hpp"
#include "TextPagerSection.hpp"

#include "VProperty.hpp"

class TextPagerLineNumberArea;
class TextEditPrivate;

class TextPagerEdit : public QAbstractScrollArea, public VPropertyObserver
{
	friend class TextPagerLineNumberArea;

	Q_OBJECT
    Q_PROPERTY(int cursorWidth READ cursorWidth WRITE setCursorWidth)
    Q_PROPERTY(bool readOnly READ readOnly WRITE setReadOnly)
    Q_PROPERTY(bool cursorVisible READ cursorVisible WRITE setCursorVisible)
    Q_PROPERTY(QString selectedText READ selectedText)
    Q_PROPERTY(bool undoAvailable READ isUndoAvailable NOTIFY undoAvailableChanged)
    Q_PROPERTY(bool redoAvailable READ isRedoAvailable NOTIFY redoAvailableChanged)
    Q_PROPERTY(int maximumSizeCopy READ maximumSizeCopy WRITE setMaximumSizeCopy)
    Q_PROPERTY(bool lineBreaking READ lineBreaking WRITE setLineBreaking)

public:
    TextPagerEdit(QWidget *parent = 0);
    ~TextPagerEdit();

    TextPagerDocument *document() const;
    void setDocument(TextPagerDocument *doc);

    int cursorWidth() const;
    void setCursorWidth(int cc);

    struct ExtraSelection
    {
        TextPagerCursor cursor;
        QTextCharFormat format;
    };

    void setExtraSelections(const QList<ExtraSelection> &selections);
    QList<ExtraSelection> extraSelections() const;

    void setSyntaxHighlighter(SyntaxHighlighter *h);
    inline SyntaxHighlighter *syntaxHighlighter() const { return syntaxHighlighters().value(0); }

    QList<SyntaxHighlighter*> syntaxHighlighters() const;
    void addSyntaxHighlighter(SyntaxHighlighter *highlighter);
    void takeSyntaxHighlighter(SyntaxHighlighter *highlighter);
    void removeSyntaxHighlighter(SyntaxHighlighter *highlighter);
    void clearSyntaxHighlighters();

    bool load(QIODevice *device, TextPagerDocument::DeviceMode mode = TextPagerDocument::Sparse, QTextCodec *codec = 0);
    bool load(const QString &fileName, TextPagerDocument::DeviceMode mode = TextPagerDocument::Sparse, QTextCodec *codec = 0);
    void paintEvent(QPaintEvent *e);
    void scrollContentsBy(int dx, int dy);

    bool moveCursorPosition(TextPagerCursor::MoveOperation op, TextPagerCursor::MoveMode = TextPagerCursor::MoveAnchor, int n = 1);
    void setCursorPosition(int pos, TextPagerCursor::MoveMode mode = TextPagerCursor::MoveAnchor);

    int viewportPosition() const;
    int cursorPosition() const;

    int textPositionAt(const QPoint &pos) const;

    bool readOnly() const;
    void setReadOnly(bool rr);

    bool lineBreaking() const;
    void setLineBreaking(bool lb);

    int maximumSizeCopy() const;
    void setMaximumSizeCopy(int max);

    QRect cursorBlockRect(const TextPagerCursor &cursor) const;
    QRect cursorRect(const TextPagerCursor &cursor) const;

    int lineNumber(int position) const;
    int columnNumber(int position) const;
    int lineNumber(const TextPagerCursor &cursor) const;
    int columnNumber(const TextPagerCursor &cursor) const;

    bool cursorVisible() const;
    void setCursorVisible(bool cc);

    QString selectedText() const;
    bool hasSelection() const;

    bool save(QIODevice *device);
    bool save();
    bool save(const QString &file);

    void setText(const QString &text);
    QString read(int pos, int size) const;
    QChar readCharacter(int index) const;

    void insert(int pos, const QString &text);
    void remove(int from, int size);

    TextPagerCursor &textCursor();
    const TextPagerCursor &textCursor() const;
    void setTextCursor(const TextPagerCursor &textCursor);

    TextPagerCursor cursorForPosition(const QPoint &pos) const;

    TextPagerSection *sectionAt(const QPoint &pos) const;

    QList<TextPagerSection*> sections(int from = 0, int size = -1, TextPagerSection::TextSectionOptions opt = 0) const;
    inline TextPagerSection *sectionAt(int pos) const { return sections(pos, 1, TextPagerSection::IncludePartial).value(0); }
    TextPagerSection *insertTextSection(int pos, int size, const QTextCharFormat &format = QTextCharFormat(),
                                   const QVariant &data = QVariant());

    void ensureCursorVisible(const TextPagerCursor &cursor, int linesMargin = 0);
    bool isUndoAvailable() const;
    bool isRedoAvailable() const;

    void setFontProperty(VProperty* p);
    void notifyChange(VProperty* p);
    void zoomIn();
    void zoomOut();

    void setLineNumberArea(TextPagerLineNumberArea *a);

    enum ActionType {
        CopyAction,
        PasteAction,
        CutAction,
        UndoAction,
        RedoAction,
        SelectAllAction
    };
    QAction *action(ActionType type) const;

public Q_SLOTS:
    void ensureCursorVisible();
    void append(const QString &text);
    void removeSelectedText();
    void copy(QClipboard::Mode mode = QClipboard::Clipboard);
    void paste(QClipboard::Mode mode = QClipboard::Clipboard);
    void cut();
    void undo();
    void redo();
    void selectAll();
    void clearSelection();

Q_SIGNALS:
    void copyAvailable(bool on);
    void textChanged();
    void selectionChanged();
    void cursorPositionChanged(int pos);
    void sectionClicked(TextPagerSection *section, const QPoint &pos);
    void undoAvailableChanged(bool on);
    void redoAvailableChanged(bool on);

protected:
    virtual void paste(int position, QClipboard::Mode mode);
    virtual void changeEvent(QEvent *e);
    virtual void keyPressEvent(QKeyEvent *e);
    virtual void keyReleaseEvent(QKeyEvent *e);
    virtual void wheelEvent(QWheelEvent *e);
    virtual void mousePressEvent(QMouseEvent *e);
    virtual void mouseDoubleClickEvent(QMouseEvent *);
    virtual void mouseMoveEvent(QMouseEvent *e);
    virtual void mouseReleaseEvent(QMouseEvent *e);
    virtual void resizeEvent(QResizeEvent *e);
#if 0 // ### not done yet
    virtual void dragEnterEvent(QDragEnterEvent *e);
    virtual void dragMoveEvent(QDragMoveEvent *e);
    virtual void dropEvent(QDropEvent *e);
#endif


private:
    void updateFont();
    void fontSizeChangedByZoom();

    void lineNumberAreaPaintEvent(QPaintEvent *e);
    int  lineNumberAreaWidth();
    void updateLineNumberArea();


    TextEditPrivate *d;
    friend class TextLayoutCacheManager;
    friend class TextEditPrivate;
    friend class TextPagerCursor;

    TextPagerLineNumberArea* lineNumArea_;
    VProperty* fontProp_;
};


class TextPagerLineNumberArea : public QWidget
{
public:
    explicit TextPagerLineNumberArea(TextPagerEdit *editor) : QWidget(editor), textEditor_ (editor) {}
    QSize sizeHint() const {QSize(textEditor_->lineNumberAreaWidth(), 0);}

protected:
    void paintEvent(QPaintEvent *event) { textEditor_->lineNumberAreaPaintEvent(event);}

private:
    TextPagerEdit *textEditor_;
};



#endif
