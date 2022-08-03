//============================================================================
// Copyright 2009- ECMWF.
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
    CursorCacheItem() = default;
protected:
    int pos_{0};
    int verticalScrollValue_{0};
    int viewportHeight_{0};
};

class OutputBrowserState;

class OutputBrowser : public QWidget
{
Q_OBJECT
    friend class OutputBrowserState;
    friend class OutputBrowserEmptyState;
    friend class OutputBrowserNormalState;
    friend class OutputBrowserReloadState;

public:
	explicit OutputBrowser(QWidget* parent);
	~OutputBrowser() override;

    void clear();
    void loadFile(VFile_ptr file);
    bool isFileLoaded();
    bool contentsChangedOnLastLoad() const {return contentsChangedOnLastLoad_;}
    void reloadBegin();
    void saveCurrentFile(QString &fileNameToSaveTo);
	void adjustHighlighter(QString fileName);
	void setFontProperty(VProperty* p);
    void updateFont();
	void gotoLine();
    void toDocStart();
    void toDocEnd();
    void toLineStart();
    void toLineEnd();
	void showSearchLine();
    void searchOnReload(bool userClickedReload);
	void zoomIn();
	void zoomOut();
    void setShowLineNumbers(bool);
    void setWordWrap(bool);
    bool isWordWrapSupported() const;
    void setSearchButtons(QToolButton* searchTb);
    void setFilterButtons(QToolButton* statusTb,QToolButton* optionTb);
    size_t sizeInBytes() const;
    VFile_ptr file() const {return file_;}

protected Q_SLOTS:
	void showConfirmSearchLabel();
    void slotRunFilter(QString,bool,bool);
    void slotRemoveFilter();

Q_SIGNALS:
    void wordWrapSupportChanged(bool);

protected:
    void clearIt();
    void loadIt(VFile_ptr);
    void reloadIt(VFile_ptr);
    void transitionTo(OutputBrowserState* state);

private:
    enum IndexType {BasicIndex=0,PagerIndex=1,HtmlIndex=2};
	void changeIndex(IndexType indexType,qint64 fileSize);
    bool isJobFile(QString fileName);
    bool isHtmlFile(QString fileName);
    void loadFilteredFile(VFile_ptr file);
    void setCursorPos(qint64 pos);
    VFile_ptr filterIt();
    void loadContents();
    void loadContentsFromDisk(QString contentsFileName, QString fileName);
    void loadContentsFromText(QString text,QString fileName,size_t dataSize, bool resetFile=true);
    bool addDeltaContents(VFile_ptr);
    bool addDeltaContentsFromText(QString deltaTxt,QString fileName, size_t fileSize);

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
    OutputBrowserState* state_{nullptr};

    //we keep a reference to it to ensure that it does not get deleted while
    //it is being displayed
    VFile_ptr file_;
    VFile_ptr contentsFile_;
    bool contentsChangedOnLastLoad_{false};

    static int minPagerTextSize_;
	static int minPagerSparseSize_;
	static int minConfirmSearchSize_;
};

#endif /* VIEWER_SRC_OUTPUTBROWSER_HPP_ */
