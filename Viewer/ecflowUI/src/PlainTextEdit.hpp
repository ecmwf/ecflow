//============================================================================
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef TEXTEDIT_HPP_
#define TEXTEDIT_HPP_

#include <QPlainTextEdit>

#include "VProperty.hpp"

class LineNumberArea;
class GotoLineDialog;

class PlainTextEdit : public QPlainTextEdit, public VPropertyObserver
{
Q_OBJECT

public:
    explicit PlainTextEdit(QWidget* parent = nullptr);
	~PlainTextEdit() override;

    void lineNumberAreaPaintEvent(QPaintEvent *event);
    int  lineNumberAreaWidth();

    bool showLineNumbers()                 {return showLineNum_;}
    void setShowLineNumbers(bool b);

    void cursorRowCol(int *row, int *col);
    QChar characterBehindCursor(QTextCursor *cursor=nullptr);

    int numLinesSelected();
    bool findString(const QString &,QTextDocument::FindFlags,bool replace=false,const QString &r=emptyString_);

    void setFontProperty(VProperty* p);
    void updateFont();
    void notifyChange(VProperty* p) override;
    bool isHyperlinkEnabled() {return hyperlinkEnabled_;}
    bool setHyperlinkEnabled(bool h);

public Q_SLOTS:
     void gotoLine();
     void slotZoomIn();
     void slotZoomOut();

private Q_SLOTS:
     void updateLineNumberAreaWidth(int newBlockCount);
     void updateLineNumberArea(const QRect &, int);
     void gotoLine(int line);

Q_SIGNALS:
    void focusRegained ();
    void focusLost();
    void fontSizeChangedByWheel();
    void hyperlinkActivated(QString link);

protected:
    void resizeEvent(QResizeEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;

private:
    void fontSizeChangedByZoom();

    bool showLineNum_{true};
    QWidget *lineNumArea_;
    int rightMargin_{2};
    bool hyperlinkEnabled_{false};
    QString lastFindString_;
    QString currentLink_;
    QTextDocument::FindFlags lastFindFlags_;
    GotoLineDialog *gotoLineDialog_{nullptr};
    static QString emptyString_;

    QColor numAreaBgCol_;
    QColor numAreaFontCol_;
    QColor numAreaSeparatorCol_;
    QColor numAreaCurrentCol_;
    VProperty *fontProp_{nullptr};
};


class LineNumberArea : public QWidget
{
public:
    explicit LineNumberArea(PlainTextEdit *editor) : QWidget(editor) {textEditor = editor;}
    QSize sizeHint() const override {return {textEditor->lineNumberAreaWidth(), 0};}

protected:
    void paintEvent(QPaintEvent *event) override { textEditor->lineNumberAreaPaintEvent(event);}

private:
    PlainTextEdit *textEditor;
};


#endif
