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
#include <QMap>

#include "VFile.hpp"

class Highlighter;
class MessageLabel;
class TextEditSearchLine;
class PlainTextEdit;
class PlainTextSearchInterface;
class TextPagerWidget;
class TextPagerSearchInterface;
class VProperty;

class OutputBrowser;

class CursorCacheItem
{
    friend class OutputBrowser;
public:
    CursorCacheItem() : pos_(0), verticalScrollValue_(0), viewportHeight_(0) {}
protected:
    int pos_;
    int verticalScrollValue_;
    int viewportHeight_;
};


class OutputBrowser : public QWidget
{
Q_OBJECT

public:
	explicit OutputBrowser(QWidget* parent);
	~OutputBrowser();

    void clear();
    void loadFile(VFile_ptr file);
    void loadText(QString text,QString fileName,bool resetFile=true);
	void adjustHighlighter(QString fileName);
	void setFontProperty(VProperty* p);
	void updateFont();
	void gotoLine();
	void showSearchLine();
	void searchOnReload(bool userClickedReload);
	void zoomIn();
	void zoomOut();
    void clearCursorCache() {cursorCache_.clear();}

protected Q_SLOTS:
	void showConfirmSearchLabel();

private:
	enum IndexType {BasicIndex=0,PagerIndex=1};
	void changeIndex(IndexType indexType,qint64 fileSize);
    bool isJobFile(QString fileName);
    void loadFile(QString fileName);
    void updateCursorFromCache(const std::string&);
    void setCursorPos(qint64 pos);

    QStackedWidget *stacked_;
	PlainTextEdit* textEdit_;
	TextPagerWidget* textPager_;
	TextEditSearchLine* searchLine_;
	Highlighter* jobHighlighter_;
	PlainTextSearchInterface *textEditSearchInterface_;
	TextPagerSearchInterface *textPagerSearchInterface_;
	MessageLabel *confirmSearchLabel_;

    //we keep a reference to it to make sure that it does not get deleted while
    //it is being displayed
    VFile_ptr file_;
    int lastPos_;
    std::string currentSourceFile_;

    QMap<std::string,CursorCacheItem> cursorCache_;

    static int minPagerTextSize_;
	static int minPagerSparseSize_;
	static int minConfirmSearchSize_;
};

#endif /* VIEWER_SRC_OUTPUTBROWSER_HPP_ */
