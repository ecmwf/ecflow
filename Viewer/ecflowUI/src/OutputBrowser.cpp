/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "OutputBrowser.hpp"

#include <cassert>

#include <QByteArray>
#include <QObject>
#include <QProcess>
#include <QVBoxLayout>
#include <QtGlobal>

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    #include <QGuiApplication>
#endif

#include "DirectoryHandler.hpp"
#include "Highlighter.hpp"
#include "HtmlEdit.hpp"
#include "MessageLabel.hpp"
#include "PlainTextEdit.hpp"
#include "PlainTextSearchInterface.hpp"
#include "TextEditSearchLine.hpp"
#include "TextFilterWidget.hpp"
#include "TextPager/TextPagerSearchInterface.hpp"
#include "TextPagerWidget.hpp"
#include "UiLog.hpp"
#include "UserMessage.hpp"
#include "VFile.hpp"

int OutputBrowser::minPagerTextSize_     = 40 * 1024 * 1024;
int OutputBrowser::minPagerSparseSize_   = 40 * 1024 * 1024;
int OutputBrowser::minConfirmSearchSize_ = 20 * 1024 * 1024;

#define UI_OUTPUTBROSWER_DEBUG_

//========================================
//
// OutputBrowserState
//
//========================================

class OutputBrowserState : public QObject {
public:
    OutputBrowserState(OutputBrowser* browser) : QObject(browser), browser_(browser) {}
    ~OutputBrowserState() override = default;

    virtual void handleClear();
    virtual void handleLoad(VFile_ptr);
    virtual void handleReloadBegin();

protected:
    OutputBrowser* browser_{nullptr};
};

class OutputBrowserEmptyState : public OutputBrowserState {
public:
    OutputBrowserEmptyState(OutputBrowser* browser);
    void handleClear() override;
};

class OutputBrowserNormalState : public OutputBrowserState {
public:
    using OutputBrowserState::OutputBrowserState;
};

class OutputBrowserReloadState : public OutputBrowserState {
public:
    using OutputBrowserState::OutputBrowserState;
    void handleLoad(VFile_ptr) override;
};

void OutputBrowserState::handleClear() {
    browser_->transitionTo(new OutputBrowserEmptyState(browser_));
}

void OutputBrowserState::handleLoad(VFile_ptr file) {
    browser_->clearIt();
    browser_->loadIt(file);
    if (browser_->isFileLoaded()) {
        browser_->transitionTo(new OutputBrowserNormalState(browser_));
    }
    else {
        browser_->transitionTo(new OutputBrowserEmptyState(browser_));
    }
}

void OutputBrowserState::handleReloadBegin() {
    browser_->transitionTo(new OutputBrowserReloadState(browser_));
}

// Empty
OutputBrowserEmptyState::OutputBrowserEmptyState(OutputBrowser* b) : OutputBrowserState(b) {
    browser_->clearIt();
}

void OutputBrowserEmptyState::handleClear() {
    browser_->clearIt();
}

// Reload
void OutputBrowserReloadState::handleLoad(VFile_ptr file) {
    browser_->reloadIt(file);
    if (browser_->isFileLoaded()) {
        browser_->transitionTo(new OutputBrowserNormalState(browser_));
    }
    else {
        browser_->transitionTo(new OutputBrowserEmptyState(browser_));
    }
}

//========================================
//
// OutputBrowser
//
//========================================

OutputBrowser::OutputBrowser(QWidget* parent) : QWidget(parent), searchTb_(nullptr) {
    auto* vb = new QVBoxLayout(this);
    vb->setContentsMargins(0, 0, 0, 0);
    vb->setSpacing(2);

    // Text filter editor
    textFilter_ = new TextFilterWidget(this);
    vb->addWidget(textFilter_);

    connect(textFilter_, SIGNAL(runRequested(QString, bool, bool)), this, SLOT(slotRunFilter(QString, bool, bool)));

    connect(textFilter_, SIGNAL(clearRequested()), this, SLOT(slotRemoveFilter()));

    connect(textFilter_, SIGNAL(closeRequested()), this, SLOT(slotRemoveFilter()));

    stacked_ = new QStackedWidget(this);
    vb->addWidget(stacked_, 1);

    confirmSearchLabel_ = new MessageLabel(this);
    confirmSearchLabel_->setShowTypeTitle(false);
    confirmSearchLabel_->setNarrowMode(true);
    vb->addWidget(confirmSearchLabel_);

    searchLine_ = new TextEditSearchLine(this);
    vb->addWidget(searchLine_);

    // Basic textedit
    textEdit_ = new PlainTextEdit(this);
    textEdit_->setReadOnly(true);
    textEdit_->setWordWrapMode(QTextOption::NoWrap);
    textEdit_->setShowLineNumbers(true);

    textEditSearchInterface_ = new PlainTextSearchInterface();
    textEditSearchInterface_->setEditor(textEdit_);

    // This highlighter only works for jobs
    jobHighlighter_ = new Highlighter(textEdit_->document(), "job");
    jobHighlighter_->setDocument(nullptr);

    // Pager for very large files
    textPager_ = new TextPagerWidget(this);
    textPager_->textEditor()->setShowLineNumbers(true);

    // textEdit_->setReadOnly(true);

    textPagerSearchInterface_ = new TextPagerSearchInterface();
    textPagerSearchInterface_->setEditor(textPager_->textEditor());

    // Html browser for html files
    htmlEdit_ = new HtmlEdit(this);

    stacked_->addWidget(textEdit_);
    stacked_->addWidget(textPager_);
    stacked_->addWidget(htmlEdit_);

    stacked_->setCurrentIndex(BasicIndex);
    searchLine_->hide();

    connect(searchLine_, SIGNAL(visibilityChanged()), this, SLOT(showConfirmSearchLabel()));

    // the textfilter is is hidden by default
    textFilter_->hide();

    // initialise state
    transitionTo(new OutputBrowserEmptyState(this));
}

OutputBrowser::~OutputBrowser() {
    delete textEditSearchInterface_;
    delete textPagerSearchInterface_;

    if (jobHighlighter_ && !jobHighlighter_->parent()) {
        delete jobHighlighter_;
    }
}

void OutputBrowser::setSearchButtons(QToolButton* searchTb) {
    searchTb_ = searchTb;
}

void OutputBrowser::setFilterButtons(QToolButton* statusTb, QToolButton* optionTb) {
    textFilter_->setExternalButtons(statusTb, optionTb);
}

//--------------------
// Public interface
//--------------------
void OutputBrowser::clear() {
    state_->handleClear();
}

// This should only be called externally when a new output is loaded
void OutputBrowser::loadFile(VFile_ptr file) {
    state_->handleLoad(file);
}

void OutputBrowser::reloadBegin() {
    state_->handleReloadBegin();
}

//---------------------------
// Internal implementations
//---------------------------
void OutputBrowser::clearIt() {
    textEdit_->clear();
    textPager_->clear();
    htmlEdit_->clear();
    file_.reset();
    contentsFile_.reset();
    lastLoadedSizeFromDisk_ = 0;
}

void OutputBrowser::loadIt(VFile_ptr file) {
    contentsChangedOnLastLoad_ = true;
    lastLoadedSizeFromDisk_    = 0;

    file_ = file;
    if (!file_) {
        return;
    }
    Q_ASSERT(!file->hasDeltaContents());
    contentsFile_ = filterIt();

    loadContents(file_->fetchMode() == VFile::LocalFetchMode);
}

void OutputBrowser::reloadIt(VFile_ptr file) {
    contentsChangedOnLastLoad_ = false;

    if (!file) {
        clearIt();
        return;
    }

    // Local cannot have deltacontents. We want to avoid recursion!
    bool local = (file_ && file_->fetchMode() == VFile::LocalFetchMode && file->fetchMode() == VFile::LocalFetchMode);
    VFile_ptr delta;

    if (local) {
        file_ = file;
    }
    else if (file->hasDeltaContents() && file_) {
        assert(file->fetchMode() != VFile::LocalFetchMode);
        delta = file;
        if (!file_->append(file)) {
            return;
        }
    }
    else {
        file_ = file;
    }

    // TODO: this is not neccesarily true when a local file is updated since at this point we do not know if
    //  the file changed
    contentsChangedOnLastLoad_ = true;
    contentsFile_              = filterIt();
    bool contentsSame          = contentsFile_.get() == file_.get();

    bool deltaAdded = false;
    if (delta && contentsFile_.get() == file_.get()) {
        deltaAdded = addDeltaContents(delta);
    }
    if (!deltaAdded) {
        loadContents(contentsSame && local);
    }
}

void OutputBrowser::loadContents(bool manageLocal) {
    if (contentsFile_ && file_) {
        if (contentsFile_->storageMode() == VFile::DiskStorage) {
            loadContentsFromDisk(
                QString::fromStdString(contentsFile_->path()), QString::fromStdString(file_->path()), manageLocal);
        }
        else {
            QString s(contentsFile_->data());
            loadContentsFromText(s, QString::fromStdString(file_->sourcePath()), contentsFile_->dataSize(), true);
        }
    }
}

void OutputBrowser::loadContentsFromDisk(QString contentsFileName, QString fileName, bool manageLocal) {
    QFile file(contentsFileName);
    if (!file.open(QIODevice::ReadOnly)) {
        lastLoadedSizeFromDisk_ = 0;
        return;
    }

    QFileInfo fInfo(file);
    qint64 fSize = fInfo.size();

    if (isHtmlFile(fileName)) {
        changeIndex(HtmlIndex, fSize);
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
        QGuiApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
#endif
        QString str = file.readAll();
        htmlEdit_->document()->setHtml(str);
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
        QGuiApplication::restoreOverrideCursor();
#endif
    }
    else if (!isJobFile(fileName) && fSize >= minPagerTextSize_) {
        changeIndex(PagerIndex, fSize);
        TextPagerDocument::DeviceMode mode =
            (fSize >= minPagerSparseSize_) ? TextPagerDocument::Sparse : TextPagerDocument::LoadAll;
        textPager_->load(contentsFileName, mode);
        lastLoadedSizeFromDisk_ = 0;
    }
    else {
        changeIndex(BasicIndex, fSize);
        adjustHighlighter(fileName);

        bool loaded = false;
        if (manageLocal && !isJobFile(fileName) && lastLoadedSizeFromDisk_ > 0) {
            if (lastLoadedSizeFromDisk_ < fSize) {
#ifdef UI_OUTPUTBROSWER_DEBUG_
                UiLog().dbg() << UI_FN_INFO << "load local file from offset=" << lastLoadedSizeFromDisk_;
#endif
                if (file.seek(lastLoadedSizeFromDisk_)) {
                    QByteArray arr = file.readAll();
                    lastLoadedSizeFromDisk_ += arr.size();
                    QString deltaTxt(arr);
                    // appendPlainText always inserts a newline so we cannot use it here
                    QTextCursor cursor = QTextCursor(textEdit_->document());
                    cursor.movePosition(QTextCursor::End);
                    cursor.insertText(deltaTxt);
                    loaded = true;
                }
            }
            else if (lastLoadedSizeFromDisk_ == fSize) {
#ifdef UI_OUTPUTBROSWER_DEBUG_
                UiLog().dbg() << UI_FN_INFO << "no need to load: local file is the same";
                loaded = true;
#endif
            }
        }

        if (!loaded) {
            QByteArray arr = file.readAll();
            if (manageLocal) {
                lastLoadedSizeFromDisk_ = arr.size();
            }
            QString str(arr);
            UiLog().dbg() << UI_FN_INFO << "load whole file";
            textEdit_->document()->setPlainText(str);
        }
    }
}

void OutputBrowser::loadContentsFromText(QString txt, QString fileName, size_t dataSize, bool /*resetFile*/) {
    // prior to the ability to save local copies of files, we reset the file_ member here;
    // but now we need to keep it so that we can save a copy of it
    // if(resetFile)
    // file_.reset();

    qint64 txtSize = dataSize;

    if (isHtmlFile(fileName)) {
        changeIndex(HtmlIndex, txtSize);
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
        QGuiApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
#endif
        htmlEdit_->document()->setHtml(txt);
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
        QGuiApplication::restoreOverrideCursor();
#endif
    }
    else if (!isJobFile(fileName) && txtSize > minPagerTextSize_) {
        changeIndex(PagerIndex, txtSize);
        textPager_->setText(txt);
    }
    else {
        changeIndex(BasicIndex, txtSize);
        adjustHighlighter(fileName);
        textEdit_->document()->setPlainText(txt);
    }

    // Set the cursor position from the cache
    // updateCursorFromCache(fileName.toStdString());
}

// try to add delta contents, not all the modes support it so the return value indicates
// if it was possible
bool OutputBrowser::addDeltaContents(VFile_ptr delta) {
    if (delta && contentsFile_ && file_) {
        if (delta->storageMode() == VFile::DiskStorage) {
            return addDeltaContentsFromDisk(
                QString::fromStdString(delta->path()), QString::fromStdString(file_->path()), file_->sizeInBytes());
        }
        else {
            QString s(delta->data());
            return addDeltaContentsFromText(s, QString::fromStdString(file_->sourcePath()), file_->sizeInBytes());
        }
    }
    return false;
}

bool OutputBrowser::addDeltaContentsFromDisk(QString deltaFileName, QString fileName, size_t fileSize) {
    if (isHtmlFile(fileName)) {
        return false;
    }
    else if (!isJobFile(fileName) || fileSize <= static_cast<size_t>(minPagerTextSize_)) {
        changeIndex(BasicIndex, fileSize);
        adjustHighlighter(fileName);
#ifdef UI_OUTPUTBROSWER_DEBUG_
        UiLog().dbg() << UI_FN_INFO << " deltaFileName=" << deltaFileName;
#endif
        QFile file(deltaFileName);
        if (file.open(QIODevice::ReadOnly)) {
            QString deltaTxt = file.readAll();
            // appendPlainText always inserts a newline so we cannot use it here
            QTextCursor cursor = QTextCursor(textEdit_->document());
            cursor.movePosition(QTextCursor::End);
            cursor.insertText(deltaTxt);
            return true;
        }
        else {
            return false;
        }
    }
    return false;
}

bool OutputBrowser::addDeltaContentsFromText(QString deltaTxt, QString fileName, size_t fileSize) {
    // at this point file_ has already been updated with the delta and fileSize contains the
    // new size. It is guaranteed_ that file_ == contentsFile_
    if (isHtmlFile(fileName)) {
        return false;
    }
    if (isJobFile(fileName) || fileSize <= static_cast<size_t>(minPagerTextSize_)) {
#ifdef UI_OUTPUTBROSWER_DEBUG_
        UiLog().dbg() << UI_FN_INFO << " text_size=" << deltaTxt.size();
#endif
        changeIndex(BasicIndex, fileSize);
        adjustHighlighter(fileName);
        // appendPlainText always inserts a newline so we cannot use it here
        QTextCursor cursor = QTextCursor(textEdit_->document());
        cursor.movePosition(QTextCursor::End);
        cursor.insertText(deltaTxt);

        return true;
    }
    return false;
}

void OutputBrowser::saveCurrentFile(QString& fileNameToSaveTo) {
    assert(file_);

    if (file_->storageMode() == VFile::DiskStorage) {
        // if we have the file on disk, then just copy it to the new location
        std::string to = fileNameToSaveTo.toStdString();
        std::string errorMessage;
        bool ok = DirectoryHandler::copyFile(file_->path(), to, errorMessage);
        if (!ok) {
            UserMessage::message(UserMessage::ERROR, true, "Failed to copy file. Reason:\n " + errorMessage);
        }
    }
    else {
        // file in memory - just dump it to file
        QFile file(fileNameToSaveTo);
        if (file.open(QFile::WriteOnly | QFile::Text)) {
            QTextStream out(&file);
            QString s(file_->data());
            out << s;
        }
        else {
            UserMessage::message(UserMessage::ERROR, true, "Failed to save file to  " + fileNameToSaveTo.toStdString());
        }
    }
}

void OutputBrowser::changeIndex(IndexType indexType, qint64 fileSize) {
    if (indexType == BasicIndex) {
        stacked_->setCurrentIndex(indexType);
        textPager_->clear();
        htmlEdit_->clear();

        // enable and init search
        if (searchTb_)
            searchTb_->setEnabled(true);
        searchLine_->setConfirmSearch(fileSize >= minConfirmSearchSize_);
        searchLine_->setSearchInterface(textEditSearchInterface_);

        // enable filter
        textFilter_->setEnabledExternalButtons(true);
    }
    else if (indexType == PagerIndex) {
        stacked_->setCurrentIndex(indexType);
        textEdit_->clear();
        htmlEdit_->clear();

        // enable and init search
        if (searchTb_)
            searchTb_->setEnabled(true);
        searchLine_->setConfirmSearch(fileSize >= minConfirmSearchSize_);
        searchLine_->setSearchInterface(textPagerSearchInterface_);

        // enable filter
        textFilter_->setEnabledExternalButtons(true);
    }
    else if (indexType == HtmlIndex) {
        stacked_->setCurrentIndex(indexType);
        textPager_->clear();
        textEdit_->clear();

        // the filtered contents should be released
        contentsFile_ = file_;

        // Disable search
        if (searchTb_)
            searchTb_->setEnabled(false);
        searchLine_->setSearchInterface(nullptr);
        searchLine_->hide();

        // Disable filter
        textFilter_->closeIt();
        textFilter_->setEnabledExternalButtons(false);
    }

    showConfirmSearchLabel();
    Q_EMIT wordWrapSupportChanged(isWordWrapSupported());
}

bool OutputBrowser::isFileLoaded() {
    return (file_ != nullptr);
}

bool OutputBrowser::isJobFile(QString fileName) {
    return fileName.contains(".job");
}

bool OutputBrowser::isHtmlFile(QString fileName) {
    return fileName.endsWith(".html");
}

void OutputBrowser::adjustHighlighter(QString fileName) {
    // For job files we set the proper highlighter
    if (isJobFile(fileName)) {
        if (!jobHighlighter_) {
            jobHighlighter_ = new Highlighter(textEdit_->document(), "job");
        }
        else if (jobHighlighter_->document() != textEdit_->document()) {
            jobHighlighter_->setDocument(textEdit_->document());
        }
    }
    else if (jobHighlighter_) {
        jobHighlighter_->setDocument(nullptr);
    }
}

void OutputBrowser::gotoLine() {
    if (stacked_->currentIndex() == BasicIndex)
        textEdit_->gotoLine();
    else if (stacked_->currentIndex() == PagerIndex)
        textPager_->gotoLine();
}

void OutputBrowser::toDocStart() {
    if (stacked_->currentIndex() == BasicIndex)
        textEdit_->toDocStart();
    else if (stacked_->currentIndex() == PagerIndex)
        textPager_->toDocStart();
}

void OutputBrowser::toDocEnd() {
    if (stacked_->currentIndex() == BasicIndex)
        textEdit_->toDocEnd();
    else if (stacked_->currentIndex() == PagerIndex)
        textPager_->toDocEnd();
}

void OutputBrowser::toLineStart() {
    if (stacked_->currentIndex() == BasicIndex)
        textEdit_->toLineStart();
    else if (stacked_->currentIndex() == PagerIndex)
        textPager_->toLineStart();
}

void OutputBrowser::toLineEnd() {
    if (stacked_->currentIndex() == BasicIndex)
        textEdit_->toLineEnd();
    else if (stacked_->currentIndex() == PagerIndex)
        textPager_->toLineEnd();
}

void OutputBrowser::showSearchLine() {
    if (searchLine_->hasInterface()) {
        searchLine_->setVisible(true);
        searchLine_->setFocus();
        searchLine_->selectAll();
    }
}

void OutputBrowser::searchOnReload(bool userClickedReload) {
    if (searchLine_->hasInterface()) {
        searchLine_->searchOnReload(userClickedReload);
    }
}

void OutputBrowser::setFontProperty(VProperty* p) {
    textEdit_->setFontProperty(p);
    textPager_->setFontProperty(p);
    htmlEdit_->setFontProperty(p);
}

void OutputBrowser::updateFont() {
    textEdit_->updateFont();
}

void OutputBrowser::zoomIn() {
    textEdit_->slotZoomIn();
    textPager_->zoomIn();
    htmlEdit_->zoomIn();
}

void OutputBrowser::zoomOut() {
    textEdit_->slotZoomOut();
    textPager_->zoomOut();
    htmlEdit_->zoomOut();
}

void OutputBrowser::setShowLineNumbers(bool st) {
    textEdit_->setShowLineNumbers(st);
    textPager_->textEditor()->setShowLineNumbers(st);
}

void OutputBrowser::setWordWrap(bool st) {
    textEdit_->setWordWrapMode(st ? QTextOption::WrapAtWordBoundaryOrAnywhere : QTextOption::NoWrap);
}

bool OutputBrowser::isWordWrapSupported() const {
    return (stacked_->currentIndex() == BasicIndex);
}

void OutputBrowser::showConfirmSearchLabel() {
    if (searchLine_->isVisible() && searchLine_->confirmSearch()) {
        confirmSearchLabel_->showWarning(searchLine_->confirmSearchText());
        // confirmSearchLabel_->show();
    }
    else {
        confirmSearchLabel_->hide();
    }
}

void OutputBrowser::setCursorPos(qint64 pos) {
    if (stacked_->currentIndex() == BasicIndex) {
        QTextCursor c = textEdit_->textCursor();
        c.setPosition(pos);
        textEdit_->setTextCursor(c);
    }
    else if (stacked_->currentIndex() == PagerIndex) {
        textPager_->textEditor()->setCursorPosition(pos);
    }
}

void OutputBrowser::slotRunFilter(QString /*filter*/, bool /*matched*/, bool /*caseSensitive*/) {
    contentsFile_ = filterIt();
    loadContents(false);
}

// Run the filter if defined. If no filtering can be performed the original file is returned.
VFile_ptr OutputBrowser::filterIt() {
    if (!textFilter_->isActive() || stacked_->currentIndex() == HtmlIndex || !file_) {
        return file_;
    }

    QString filter     = textFilter_->filterText();
    bool matched       = textFilter_->isMatched();
    bool caseSensitive = textFilter_->isCaseSensitive();

    if (filter.isEmpty()) {
        return file_;
    }

    VFile_ptr fTarget = VFile::createTmpFile(true);
    VFile_ptr fSrc    = VFile_ptr();

    Q_ASSERT(file_);
    // file is on disk - we use it as it is
    if (file_->storageMode() == VFile::DiskStorage) {
        fSrc = file_;
    }
    // file in memory - just dump it to the tmp file
    else {
        fSrc = VFile::createTmpFile(true);
        QFile file(QString::fromStdString(fSrc->path()));
        if (file.open(QFile::WriteOnly | QFile::Text)) {
            QTextStream out(&file);
            QString s(file_->data());
            out << s;
        }
    }

    // At this point point fSrc must contain the text to filter
    QProcess proc;
    proc.setStandardOutputFile(QString::fromStdString(fTarget->path()));

    // Run grep to filter fSrc into fTarget
    QString extraOptions;
    if (!matched)
        extraOptions += " -v ";
    if (!caseSensitive)
        extraOptions += " -i ";

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    QGuiApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
#endif

    proc.start("/bin/sh",
               QStringList() << "-c"
                             << "grep " + extraOptions + " -e \'" + filter + "\' " +
                                    QString::fromStdString(fSrc->path()));

#ifdef UI_OUTPUTBROSWER_DEBUG_
    #if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    UiLog().dbg() << "args=" << proc.arguments().join(" ");
    #endif
#endif

    if (!proc.waitForStarted(1000)) {
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
        QGuiApplication::restoreOverrideCursor();
#endif
        QString errStr;
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
        errStr = "Failed to start text filter using command:<br> \'" + proc.program() + " " +
                 proc.arguments().join(" ") + "\'";
#else
        errStr = "Failed to start grep command!";
#endif
        UserMessage::message(UserMessage::ERROR, true, errStr.toStdString());
        fTarget.reset();
        return file_;
    }

    proc.waitForFinished(60000);

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    QGuiApplication::restoreOverrideCursor();
#endif

    QString errStr = proc.readAllStandardError();
    if (proc.exitStatus() == QProcess::NormalExit && errStr.isEmpty()) {
        // oriFile_=file_;
        bool empty = fTarget->isEmpty();
        textFilter_->setStatus(empty ? (TextFilterWidget::NotFoundStatus) : (TextFilterWidget::FoundStatus));
        if (textFilter_->needNumberOfLines() && !empty) {
            textFilter_->setNumberOfLines(fTarget->numberOfLines());
        }
        return fTarget;
        // loadFilteredFile(fTarget);
    }
    else {
        QString msg;
        UI_FUNCTION_LOG
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
        msg = "Failed to filter output file using command:<br> \'" + proc.program() + " " + proc.arguments().join(" ") +
              "\'";
#else
        msg = "Failed to run grep command!";
#endif
        if (!errStr.isEmpty())
            msg += "<br>Error message: " + errStr;

        UserMessage::message(UserMessage::ERROR, true, msg.toStdString());
        textFilter_->setStatus(TextFilterWidget::NotFoundStatus);
        fTarget.reset(); // delete
    }

    return file_;
}

void OutputBrowser::slotRemoveFilter() {
    if (stacked_->currentIndex() == HtmlIndex)
        return;

    contentsFile_ = file_;
    loadContents(false);
}

size_t OutputBrowser::sizeInBytes() const {
    return (file_) ? file_->sizeInBytes() : 0;
}

void OutputBrowser::transitionTo(OutputBrowserState* state) {
#ifdef MVQFEATURESELECTORITEM_DEBUG_
    std::cout << " MvQFeatureSelectorItem::transitionTo " << typeid(*state).name() << "\n";
#endif
    if (state_ != nullptr) {
        state_->deleteLater();
    }
    state_ = state;
}
