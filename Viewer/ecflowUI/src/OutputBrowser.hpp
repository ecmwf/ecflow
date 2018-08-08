//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================
#ifndef VIEWER_SRC_OUTPUTBROWSER_HPP_
#define VIEWER_SRC_OUTPUTBROWSER_HPP_

#include <QLineEdit>
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
class TextFilterWidget;
class QToolButton;
class HtmlEdit;

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
	~OutputBrowser() override;

    void clear();
    void loadFile(VFile_ptr file);
    bool isFileLoaded();
    void saveCurrentFile(QString &fileNameToSaveTo);
	void adjustHighlighter(QString fileName);
	void setFontProperty(VProperty* p);
    void updateFont();
	void gotoLine();
	void showSearchLine();
    void searchOnReload(bool userClickedReload);
	void zoomIn();
	void zoomOut();
    void setSearchButtons(QToolButton* searchTb);
    void setFilterButtons(QToolButton* statusTb,QToolButton* optionTb);

protected Q_SLOTS:
	void showConfirmSearchLabel();
    void slotRunFilter(QString,bool,bool);
    void slotRemoveFilter();

private:
    enum IndexType {BasicIndex=0,PagerIndex=1,HtmlIndex=2};
	void changeIndex(IndexType indexType,qint64 fileSize);
    bool isJobFile(QString fileName);
    bool isHtmlFile(QString fileName);
    void loadFile(QString fileName);
    void loadText(QString text,QString fileName,bool resetFile=true);
    void loadFilteredFile(VFile_ptr file);
    void setCursorPos(qint64 pos);

    QStackedWidget *stacked_;
	PlainTextEdit* textEdit_;
	TextPagerWidget* textPager_;
    HtmlEdit* htmlEdit_;
	TextEditSearchLine* searchLine_;
    TextFilterWidget* textFilter_;
    Highlighter* jobHighlighter_;
	PlainTextSearchInterface *textEditSearchInterface_;
	TextPagerSearchInterface *textPagerSearchInterface_;
	MessageLabel *confirmSearchLabel_;
    QToolButton* searchTb_;

    //we keep a reference to it to make sure that it does not get deleted while
    //it is being displayed
    VFile_ptr file_;
    VFile_ptr oriFile_;

    static int minPagerTextSize_;
	static int minPagerSparseSize_;
	static int minConfirmSearchSize_;
};

#endif /* VIEWER_SRC_OUTPUTBROWSER_HPP_ */
