//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef TEXTEDIT_HPP_
#define TEXTEDIT_HPP_

#include <QPlainTextEdit>

class LineNumberArea;

class TextEdit : public QPlainTextEdit
{
Q_OBJECT

public:
    explicit TextEdit(QWidget* parent = 0);
	~TextEdit();

    void lineNumberAreaPaintEvent(QPaintEvent *event);
    int  lineNumberAreaWidth();

    bool showLineNumbers()                 {return showLineNum_;}
    void setShowLineNumbers(bool b);

    void cursorRowCol(int *row, int *col);
    QChar characterBehindCursor(QTextCursor *cursor=0);

    int numLinesSelected();
    bool findString(const QString &,QTextDocument::FindFlags,bool replace=false,const QString &r=emptyString_);

private Q_SLOTS:
     void updateLineNumberAreaWidth(int newBlockCount);
     void updateLineNumberArea(const QRect &, int);

Q_SIGNALS:
    void focusRegained ();
    void focusLost();

protected:
    void resizeEvent(QResizeEvent *event);
    void focusInEvent(QFocusEvent *event);
    void focusOutEvent(QFocusEvent *event);

private:
    bool showLineNum_;
    QWidget *lineNumArea_;
    int rightMargin_;
    QString  lastFindString_;
    QTextDocument::FindFlags lastFindFlags_;
    static QString emptyString_;
};


class LineNumberArea : public QWidget
{
public:
    explicit LineNumberArea(TextEdit *editor) : QWidget(editor) {textEditor = editor;}
    QSize sizeHint() const {return QSize(textEditor->lineNumberAreaWidth(), 0);}

protected:
    void paintEvent(QPaintEvent *event) { textEditor->lineNumberAreaPaintEvent(event);}

private:
    TextEdit *textEditor;
};


#endif
