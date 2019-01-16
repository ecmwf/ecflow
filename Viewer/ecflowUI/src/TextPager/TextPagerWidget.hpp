//============================================================================
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef VIEWER_SRC_TEXTPAGERWIDGET_HPP_
#define VIEWER_SRC_TEXTPAGERWIDGET_HPP_

#include "TextPagerEdit.hpp"

class GotoLineDialog;

class TextPagerWidget : public QWidget //TextPagerEdit
{
    Q_OBJECT
public:
	TextPagerWidget(QWidget *parent = nullptr);

	TextPagerEdit* textEditor() const {return textEditor_;}
	void clear();
    bool load(const QString &fileName, TextPagerDocument::DeviceMode mode = TextPagerDocument::Sparse);
    void setText(const QString& txt);
    
    void setFontProperty(VProperty* p);
    void zoomIn();
    void zoomOut();
    void gotoLine();
    
    //void mouseMoveEvent(QMouseEvent *e);
	//void timerEvent(QTimerEvent *e);
protected Q_SLOTS:
    void gotoLine(int);
    
Q_SIGNALS:
    void cursorCharacter(const QChar &ch);

private:

    bool doLineNumbers{true};
    QBasicTimer appendTimer, changeSelectionTimer;
    TextPagerEdit *textEditor_;

    TextPagerLineNumberArea *lineNumArea_;
    GotoLineDialog *gotoLineDialog_{nullptr};
};


#endif /* VIEWER_SRC_TEXTPAGER_TEXTPAGERWIDGET_HPP_ */
