/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef VIEWER_SRC_TEXTPAGERWIDGET_HPP_
#define VIEWER_SRC_TEXTPAGERWIDGET_HPP_

#include "TextPagerEdit.hpp"

class GotoLineDialog;

class TextPagerWidget : public QWidget // TextPagerEdit
{
    Q_OBJECT
public:
    explicit TextPagerWidget(QWidget* parent = nullptr);

    TextPagerEdit* textEditor() const { return textEditor_; }
    void clear();
    bool load(const QString& fileName, TextPagerDocument::DeviceMode mode = TextPagerDocument::Sparse);
    void setText(const QString& txt);

    void setFontProperty(VProperty* p);
    void zoomIn();
    void zoomOut();
    void gotoLine();
    void toDocStart();
    void toDocEnd();
    void toLineStart();
    void toLineEnd();

    // void mouseMoveEvent(QMouseEvent *e);
    // void timerEvent(QTimerEvent *e);
protected Q_SLOTS:
    void gotoLine(int);

Q_SIGNALS:
    void cursorCharacter(const QChar& ch);

private:
    bool doLineNumbers{true};
    QBasicTimer appendTimer, changeSelectionTimer;
    TextPagerEdit* textEditor_;

    TextPagerLineNumberArea* lineNumArea_;
    GotoLineDialog* gotoLineDialog_{nullptr};
};

#endif /* VIEWER_SRC_TEXTPAGER_TEXTPAGERWIDGET_HPP_ */
