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
class TextEdit;
class TextPagerWidget;
class VProperty;

class OutputBrowser : public QWidget
{
public:
	OutputBrowser(QWidget* parent);

	void clear();
	void loadFile(QString fileName);
	void loadText(QString text);
	void adjustHighlighter(QString fileName);
	void setFontProperty(VProperty* p);
	void updateFont();
	void gotoLine();
	bool automaticSearchForKeywords();
	void zoomIn();
	void zoomOut();

private:
	enum {BasicIndex=0,PagerIndex=1};

	QStackedWidget *stacked_;
	TextEdit* textEdit_;
	TextPagerWidget* textPager_;
	Highlighter* jobHighlighter_;
};

#endif /* VIEWER_SRC_OUTPUTBROWSER_HPP_ */
