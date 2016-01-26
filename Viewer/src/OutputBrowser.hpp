//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================
#ifndef VIEWER_SRC_OUTPUTBROWSER_HPP_
#define VIEWER_SRC_OUTPUTBROWSER_HPP_

#include <QStackedWidget>

class Highlighter;
class MessageLabel;
class TextEditSearchLine;
class PlainTextEdit;
class PlainTextSearchInterface;
class TextPagerWidget;
class TextPagerSearchInterface;
class VProperty;

class OutputBrowser : public QWidget
{
Q_OBJECT

public:
	explicit OutputBrowser(QWidget* parent);
	~OutputBrowser();

	void clear();
	void loadFile(QString fileName);
	void loadText(QString text,QString fileName);
	void adjustHighlighter(QString fileName);
	void setFontProperty(VProperty* p);
	void updateFont();
	void gotoLine();
	void showSearchLine();
	void searchOnReload(bool userClickedReload);
	void zoomIn();
	void zoomOut();

protected Q_SLOTS:
	void showConfirmSearchLabel();

private:
	enum IndexType {BasicIndex=0,PagerIndex=1};
	void changeIndex(IndexType indexType,qint64 fileSize);
    bool isJobFile(QString fileName);
    
	QStackedWidget *stacked_;
	PlainTextEdit* textEdit_;
	TextPagerWidget* textPager_;
	TextEditSearchLine* searchLine_;
	Highlighter* jobHighlighter_;
	PlainTextSearchInterface *textEditSearchInterface_;
	TextPagerSearchInterface *textPagerSearchInterface_;
	MessageLabel *confirmSearchLabel_;
	static int minPagerTextSize_;
	static int minPagerSparseSize_;
	static int minConfirmSearchSize_;
};

#endif /* VIEWER_SRC_OUTPUTBROWSER_HPP_ */
