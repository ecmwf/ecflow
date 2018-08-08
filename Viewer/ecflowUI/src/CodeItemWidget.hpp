//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef CODEITEMWIDGET_HPP_
#define CODEITEMWIDGET_HPP_

#include <QWidget>

#include "ui_CodeItemWidget.h"

class CodeItemWidget : public QWidget, protected Ui::CodeItemWidget
{
Q_OBJECT

public:
	explicit CodeItemWidget(QWidget *parent=nullptr);
	~CodeItemWidget() override;

protected Q_SLOTS:
	void on_searchTb__clicked();
	void on_gotoLineTb__clicked();
	void on_fontSizeUpTb__clicked();
	void on_fontSizeDownTb__clicked();
    void on_reloadTb__clicked();
    void on_copyPathTb__clicked();

Q_SIGNALS:
	void editorFontSizeChanged();

protected:
	void removeSpacer();
    virtual void reloadRequested()=0;
    void setCurrentFileName(const std::string&);
    void clearCurrentFileName();

private:
    std::string currentFileName_;


};

#endif

