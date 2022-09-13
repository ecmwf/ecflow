//============================================================================
// Copyright 2009- ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "OutputItemWidget.hpp"

#include "OutputDirProvider.hpp"
#include "OutputFetchInfo.hpp"
#include "OutputFileProvider.hpp"
#include "OutputModel.hpp"
#include "PlainTextEdit.hpp"
#include "ServerHandler.hpp"
#include "TextFormat.hpp"
#include "TextPagerEdit.hpp"
#include "VConfig.hpp"
#include "ViewerUtil.hpp"
#include "VNode.hpp"
#include "VReply.hpp"
#include "UiLog.hpp"
#include "UserMessage.hpp"

#include "OutputDirWidget.hpp"

#include <QtGlobal>
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
#include <QGuiApplication>
#else
#include <QApplication>
#endif

#include <QClipboard>
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QItemSelectionModel>
#include <QMenu>
#include <QMovie>
#include <QTime>
#include <QWidgetAction>
#include <QFileDialog>

#define _UI_OUTPUTITEMWIDGET_DEBUG

//========================================================
//
// OutputItemWidget
//
//========================================================

OutputItemWidget::OutputItemWidget(QWidget *parent) :
	QWidget(parent)
{
    //We try to keep the contents when clicking away
    //tryToKeepContents_=true;

    setupUi(this);

	// The file contents
    messageLabel_->hide();
    messageLabel_->setShowTypeTitle(false);
    warnLabel_->hide();
    fileLabel_->setProperty("fileInfo","1");

	infoProvider_=new OutputFileProvider(this);

    //The dir listing
    connect(dirW_, SIGNAL(updateRequested()),
            this, SLOT(slotUpdateDirs()));

    connect(dirW_,SIGNAL(itemSelected()),
            this,SLOT(slotDirItemSelected()));

    connect(dirW_,SIGNAL(closedByButton()),
            this,SLOT(adjustShowDirTb()));

    connect(dirW_,SIGNAL(shrinkRequested()),
            this,SLOT(shrinkDirPanel()));

    dirProvider_=new OutputDirProvider(this);

    //show dir panel
    showDirProp_ = VConfig::instance()->find("panel.output.showDirPanel");
    Q_ASSERT(showDirProp_);
    bool showDirSt = showDirProp_->value().toBool();
    showDirTb_->setChecked(showDirSt);
    slotShowDir(showDirSt);

    connect(showDirTb_, SIGNAL(clicked(bool)),
            this,SLOT(slotShowDir(bool)));

	//Set splitter's initial size.
	int wHeight=size().height();
	if(wHeight > 100)
	{
		QList<int> sizes;
		sizes << wHeight-80 << 80;
		splitter_->setSizes(sizes);
	}

	//Editor font
	browser_->setFontProperty(VConfig::instance()->find("panel.output.font"));

    fetchInfo_=new OutputFileFetchInfo(this);
    auto* fetchInfoAction=new QWidgetAction(this);
    fetchInfoAction->setDefaultWidget(fetchInfo_);
    fetchInfoTb_->addAction(fetchInfoAction);

    filterTb_->setProperty("strip","first");
    filterOptionTb_->setProperty("strip","last");

    //Init filter in output browser
    browser_->setFilterButtons(filterTb_,filterOptionTb_);

    browser_->setSearchButtons(searchTb_);

    //line number
    lineNumProp_ = VConfig::instance()->find("panel.output.showLineNumber");
    Q_ASSERT(lineNumProp_);
    bool showLineNum = lineNumProp_->value().toBool();
    actionLineNumber_->setChecked(showLineNum);
    slotLineNumber(showLineNum);

    // text wrapline
    wordWrapProp_ = VConfig::instance()->find("panel.output.wordWrap");
    Q_ASSERT(wordWrapProp_);
    bool useWordWrap = wordWrapProp_->value().toBool();
    wordWrapTb_->setChecked(useWordWrap);
    on_wordWrapTb__clicked(useWordWrap);

    connect(browser_, SIGNAL(wordWrapSupportChanged(bool)),
            this, SLOT(slotWordWrapSupportChanged(bool)));

    slotWordWrapSupportChanged(browser_->isWordWrapSupported());

    // the icon for this button changes according to state
    expandFileInfoTb_->setIcon(ViewerUtil::makeExpandIcon(false));
    expandFileInfoTb_->setMaximumSize(QSize(16, 16));
    expandFileInfoProp_ = VConfig::instance()->find("panel.output.expandFileInfo");
    Q_ASSERT(expandFileInfoProp_);
    bool expandSt = expandFileInfoProp_->value().toBool();
    expandFileInfoTb_->setChecked(expandSt);
    on_expandFileInfoTb__clicked(expandSt);

    // More actions
    auto menu=new QMenu(this);
    menu->addAction(actionLineNumber_);
    menu->addAction(actionSaveFileAs_);
    menu->addAction(actionCopyFilePath_);
    auto sep = new QAction(this);
    sep->setSeparator(true);
    menu->addAction(sep);
    menu->addAction(actionLoadWhole_);
    menu->addAction(actionLoadCurrentJobout_);

    moreActionTb_->setMenu(menu);

    connect(actionLineNumber_, SIGNAL(toggled(bool)),
            this, SLOT(slotLineNumber(bool)));

    connect(actionSaveFileAs_, SIGNAL(triggered()),
            this, SLOT(slotSaveFileAs()));

    connect(actionCopyFilePath_, SIGNAL(triggered()),
            this, SLOT(slotCopyPath()));

    connect(actionLoadWhole_, SIGNAL(triggered()),
            this, SLOT(slotLoadWholeFile()));

    connect(actionLoadCurrentJobout_, SIGNAL(triggered()),
            this, SLOT(loadCurrentJobout()));
}

OutputItemWidget::~OutputItemWidget()
= default;

QWidget* OutputItemWidget::realWidget()
{
	return this;
}

void OutputItemWidget::reload(VInfo_ptr info)
{
    assert(active_);

    if(suspended_)
        return;

    clearContents();

    //set the info
    adjust(info);

    userClickedReload_ = false;

    //info must be a node
    if(info_ && info_->isNode() && info_->node())
	{
        //Get file contents
        infoProvider_->info(info_);

        //Get dir contents
        dirW_->reload();
	}
}

void OutputItemWidget::clearContents()
{
    InfoPanelItem::clear();
    dirW_->clear();
    messageLabel_->hide();
    messageLabel_->stopProgress();
    fileLabel_->clear();      
    browser_->clear();
    reloadTb_->setEnabled(true);
    userClickedReload_ = false;
    fetchInfo_->clearInfo();
    submittedWarning_=false;
}

void OutputItemWidget::updateState(const FlagSet<ChangeFlag>& flags)
{
    if(flags.isSet(SelectedChanged))
    {
        if(selected_ && !suspended_)
        {            
            dirW_->reload();
        }
        //If unselected we stop the automatic dir update
        else
        {
            dirW_->suspendAutoUpdate();
        }
    }

    if(flags.isSet(SuspendedChanged))
    {
        //Suspend
        if(suspended_)
        {
            reloadTb_->setEnabled(false);
            dirW_->clear();
        }
        //Resume
        else
        {
            if(info_ && info_->node())
            {
                reloadTb_->setEnabled(true);
                dirW_->clear();
                if(selected_)
                {
                    dirW_->reload();
                }
            }
            else
            {
                clearContents();
            }
        }
    }
}

void OutputItemWidget::infoReady(VReply* reply)
{
    auto* op=static_cast<OutputFileProvider*>(infoProvider_);

    //------------------------
    // From output provider
    //------------------------

    if(reply->sender() == infoProvider_)
    {
        messageLabel_->stopProgress();

        //For some unknown reason the textedit font, although it is properly set in the constructor,
        //is reset to default when we first call infoready. So we need to set it again!!
        browser_->updateFont();

        //TODO: make it possible to show warning and info at the same time
        bool hasMessage=false;
        submittedWarning_=false;
        //auto* op=static_cast<OutputFileProvider*>(infoProvider_);
        if(reply->fileName() == op->joboutFileName() && !op->isTryNoZero(reply->fileName()) &&
           info_ && info_->isNode() && info_->node() && info_->node()->isSubmitted())
        {
            hasMessage=true;
            submittedWarning_=true;
            messageLabel_->showWarning("This is the current job output (as defined by variable ECF_JOBOUT), but \
                   because the node status is <b>submitted</b> it may contain the output from a previous run!");
        }
        else
        {
            if(reply->hasWarning())
            {
                messageLabel_->showWarning(QString::fromStdString(reply->warningText()));
                hasMessage=true;
            }
            else if(reply->hasInfo())
            {
                messageLabel_->showInfo(QString::fromStdString(reply->infoText()));
                hasMessage=true;
            }
        }

        // TODO: does it make sense?
        browser_->adjustHighlighter(QString::fromStdString(reply->fileName()));

        // Load the file in the browser
        VFile_ptr f=reply->tmpFile();
        if(f)
        {
            browser_->loadFile(f);
            if(f->storageMode() == VFile::DiskStorage)
                hasMessage=false;
        }

        if(!hasMessage)
        {
            messageLabel_->hide();
        }

        //Update the file label. The reply might only contain a delta!
        fileLabel_->update(reply, browser_->file());

        //Search for some keywords in the current jobout
        if(f && browser_->contentsChangedOnLastLoad()) {
            bool searched = false;

            //We do not have dir info so the file must be the jobout
            if(dirW_->isEmpty()) {
                searchOnReload();
                searched = true;
            //We have dir info
            } else {
                //auto* op=static_cast<OutputFileProvider*>(infoProvider_);
                if(reply->fileName() == op->joboutFileName())
                {
                    searchOnReload();
                    searched = true;
                }
            }

            // if the search is not performed but the user clicked the reload
            // button we always go to the end of the document
            if (!searched && userClickedReload_) {
                browser_->toDocEnd();
            }
        }

        userClickedReload_ = false;
        reloadTb_->setEnabled(true);

#if 0
        //If we got a local file or a file via the logserver we restart the dir update timer
        if(!suspended_ &&
           (reply->fileReadMode() == VReply::LocalReadMode ||
            reply->fileReadMode() == VReply::LogServerReadMode))
        {
            dirW_->enableAutoUpdate(true);
        }
#endif
        //Update the selection in the dir list according to the file
        dirW_->adjustCurrentSelection(f);
#if 0
        if(reply->tmpFile() && reply->fileReadMode() == VReply::LocalReadMode &&
            info_ && !info_->server()->isLocalHost())
        {
            QString msg="The output file was read <b>from disk</b> but the server's \
                       host (" + QString::fromStdString(info_->server()->host()) +
                       ") is not running on the local machine. If the path is machine-specific (e.g. /tmp) \
                       and there exists a file with the same path on the local machine, then\
                       this will have been read instead.";

            warnLabel_->showWarning(msg);
        }
#endif
        fetchInfo_->setInfo(reply,info_);
        fetchInfo_->setError(reply->errorTextVec());
    }

    //------------------------
    // From output dir provider
    //------------------------
    else
    {    
        dirW_->load(reply, op->joboutFileName());
        dirW_->adjustCurrentSelection(browser_->file());
        op->setDirectories(reply->directories());
    }
}

void OutputItemWidget::infoProgress(VReply* reply)
{
	messageLabel_->showInfo(QString::fromStdString(reply->infoText()));
}

void OutputItemWidget::infoProgressStart(const std::string& text,int max)
{
    messageLabel_->startDelayedProgress(QString::fromStdString(text), max);
}

void OutputItemWidget::infoProgressUpdate(const std::string& text,int value)
{
    messageLabel_->progress(QString::fromStdString(text),value);
}

void OutputItemWidget::infoProgressStop()
{
    messageLabel_->stopProgress();
}

void OutputItemWidget::infoFailed(VReply* reply)
{
    //File
    if(reply->sender() == infoProvider_)
	{
		QString s=QString::fromStdString(reply->errorText());
        int lineCnt = s.count("\n");

        // We onny show the first two lines or the error in label
        if (lineCnt > 2) {
            auto lst = s.split("\n");
            if (lst.count() >= 3) {
                messageLabel_->showError(lst[0] + "\n" + lst[1] + "\n...\nSee the rest of the error message via the <b>Detailed info</b> button");
            }
        }

        messageLabel_->stopProgress();
        submittedWarning_=false;

		//Update the file label
		fileLabel_->update(reply);

        userClickedReload_ = false;
        reloadTb_->setEnabled(true);       

        fetchInfo_->setInfo(reply,info_);
        fetchInfo_->setError(reply->errorTextVec());
	}
    //Directories
    else
    {
        auto* op=static_cast<OutputFileProvider*>(infoProvider_);
        dirW_->failed(reply, op->joboutFileName());
    }
}

void OutputItemWidget::on_reloadTb__clicked()
{
	userClickedReload_ = true;
    reloadTb_->setEnabled(false);
    reloadCurrentFile(false);
}

void OutputItemWidget::slotLoadWholeFile()
{
    userClickedReload_ = true;
    reloadTb_->setEnabled(false);
    reloadCurrentFile(true);
}

void OutputItemWidget::loadCurrentJobout()
{
    messageLabel_->hide();
    messageLabel_->stopProgress();
    fileLabel_->clear();
    browser_->clear();
    dirW_->clear();

    fetchInfo_->clearInfo();

    //Get the latest file contents
    auto* op=static_cast<OutputFileProvider*>(infoProvider_);
    Q_ASSERT(op);
    op->fetchCurrentJobout(false);

    // get the directory listing
    dirW_->reload();
}

// called when the reload button is clicked
void OutputItemWidget::reloadCurrentFile(bool wholeFile)
{
    if(!info_) {
        return;
    }

    bool doReload = true;
    bool useCache = !doReload;

    messageLabel_->hide();
    messageLabel_->stopLoadLabel();
    messageLabel_->stopProgress();
    fetchInfo_->clearInfo();

    // the file to fetch
    std::string fPath;

    auto* op=static_cast<OutputFileProvider*>(infoProvider_);
    size_t deltaPos = 0;
    auto f = browser_->file();

    // if the browser is empty we fetch the current jobout file
    if (!f) {
        fileLabel_->clear();
        browser_->clear();
        fPath = op->joboutFileName();
#ifdef _UI_OUTPUTITEMWIDGET_DEBUG
        UiLog().dbg()  << UI_FN_INFO << "load jobout - fPath=" << fPath;
#endif
        op->fetchFile(fPath, 0, useCache);
    } else {
        fPath = f->sourcePath();
        if (wholeFile) {
            browser_->clear();
        } else {
            browser_->reloadBegin();
            deltaPos = browser_->sizeInBytes();
        }
#ifdef _UI_OUTPUTITEMWIDGET_DEBUG
        UiLog().dbg()  << UI_FN_INFO << "reload - mode=" << f->fetchMode() << " fPath=" << fPath;
#endif
        op->fetchFileForMode(f, deltaPos, useCache);
    }
    // get the directory listing
    dirW_->reload();

}

// called when an item is selected in the dir listing
void OutputItemWidget::loadCurrentDirItemFile()
{
    if(!info_) {
        return;
    }

    bool doReload = false;
    bool useCache = !doReload;

    messageLabel_->hide();
    messageLabel_->stopLoadLabel();
    messageLabel_->stopProgress();
    fileLabel_->clear();
    browser_->clear();
    fetchInfo_->clearInfo();

    std::string fPath;
    VDir::FetchMode mode;
    bool hasSelection = dirW_->currentSelection(fPath, mode);

    if (hasSelection) {
        auto* op=static_cast<OutputFileProvider*>(infoProvider_);

#ifdef _UI_OUTPUTITEMWIDGET_DEBUG
        UiLog().dbg()  << UI_FN_INFO << " mode=" << mode << " fPath=" << fPath;
#endif
        // if the fetchmode is not defined we use the normal fetch policy
        if(mode == VDir::NoFetchMode) {
            op->fetchFile(fPath, 0, useCache);
        // otherwise we use the given fetch mode
        } else {
            op->fetchFileForMode(fPath, mode, useCache);
        }
        // get the directory listing
        dirW_->reload();
    }
}

bool OutputItemWidget::isJoboutLoaded() const
{
    auto* op=static_cast<OutputFileProvider*>(infoProvider_);
    Q_ASSERT(op);
    auto f = browser_->file();
    if (f) {
        return f->sourcePath() == op->joboutFileName();
    }
    return false;
}

//------------------------------------
// Directory contents
//------------------------------------

// should only be invoked from dirW_
void OutputItemWidget::slotUpdateDirs()
{
    dirProvider_->info(info_);
}

//This slot is called when a file item is selected in the dir view
void OutputItemWidget::slotDirItemSelected()
{
    loadCurrentDirItemFile();
}

void OutputItemWidget::slotShowDir(bool st)
{
    dirW_->showIt(st);
    Q_ASSERT(showDirProp_);
    showDirProp_->setValue(st);
}

void OutputItemWidget::adjustShowDirTb()
{
    showDirTb_->setChecked(dirW_->isNotInDisabledState());
    Q_ASSERT(showDirProp_);
    showDirProp_->setValue(showDirTb_->isChecked());
}

void OutputItemWidget::shrinkDirPanel()
{
    int wHeight=size().height();
    if(wHeight > 100)
    {
        QList<int> sizes = {wHeight,0};
        splitter_->setSizes(sizes);
    }
}

//---------------------------------------------
// Search
//---------------------------------------------

void OutputItemWidget::on_searchTb__clicked()
{
	browser_->showSearchLine();
}

void OutputItemWidget::on_gotoLineTb__clicked()
{
	browser_->gotoLine();
}

// Called when we load a new node's information into the panel, or
// when we move to the panel from another one.
// If the search box is open, then search for the first matching item;
// otherwise, search for a pre-configured list of keywords. If none
// are found, and the user has clicked on the 'reload' button then
// we just go to the last line of the output
void OutputItemWidget::searchOnReload()
{
	browser_->searchOnReload(userClickedReload_);
}

//-----------------------------------------
// Fontsize management
//-----------------------------------------

void OutputItemWidget::on_fontSizeUpTb__clicked()
{
	//We need to call a custom slot here instead of "zoomIn"!!!
	browser_->zoomIn();
}

void OutputItemWidget::on_fontSizeDownTb__clicked()
{
	//We need to call a custom slot here instead of "zoomOut"!!!
	browser_->zoomOut();
}

//-----------------------------------------
// Show line number
//-----------------------------------------

void OutputItemWidget::slotLineNumber(bool st)
{
    browser_->setShowLineNumbers(st);
    Q_ASSERT(lineNumProp_);
    lineNumProp_->setValue(st);
}

//-----------------------------------------
// Word wrap
//-----------------------------------------

void OutputItemWidget::on_wordWrapTb__clicked(bool st)
{
    browser_->setWordWrap(st);
    Q_ASSERT(wordWrapProp_);
    wordWrapProp_->setValue(st);
}

void OutputItemWidget::slotWordWrapSupportChanged(bool st)
{
    wordWrapTb_->setEnabled(st);
}

//-----------------------------------------
// Navigation
//-----------------------------------------

void OutputItemWidget::on_toStartTb__clicked()
{
    //We need to call a custom slot here instead of "zoomOut"!!!
    browser_->toDocStart();
}

void OutputItemWidget::on_toEndTb__clicked()
{
    //We need to call a custom slot here instead of "zoomOut"!!!
    browser_->toDocEnd();
}

void OutputItemWidget::on_toLineStartTb__clicked()
{
    //We need to call a custom slot here instead of "zoomOut"!!!
    browser_->toLineStart();
}

void OutputItemWidget::on_toLineEndTb__clicked()
{
    //We need to call a custom slot here instead of "zoomOut"!!!
    browser_->toLineEnd();
}

//-----------------------------------------
// Save local copy of file
//-----------------------------------------

void OutputItemWidget::slotSaveFileAs()
{
	if (browser_->isFileLoaded())
	{
		QString fileName = QFileDialog::getSaveFileName(this);
		if (fileName.isEmpty())
			return;

		browser_->saveCurrentFile(fileName);
	}
	else
	{
        UserMessage::message(UserMessage::INFO,true,"No file loaded!");
	}
}

//-----------------------------------------
// Copy file path
//-----------------------------------------

void OutputItemWidget::slotCopyPath()
{
    auto f = browser_->file();
    if (f) {
        auto fPath = f->sourcePath();
        if(!fPath.empty())
        {
            QString txt=QString::fromStdString(fPath);

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
            QClipboard* cb=QGuiApplication::clipboard();
            cb->setText(txt, QClipboard::Clipboard);
            cb->setText(txt, QClipboard::Selection);
#else
            QClipboard* cb=QApplication::clipboard();
            cb->setText(txt, QClipboard::Clipboard);
            cb->setText(txt, QClipboard::Selection);
#endif
        }
    }
}

//-------------------------
// File info label
//-------------------------

void OutputItemWidget::on_expandFileInfoTb__clicked(bool st)
{
    Q_ASSERT(expandFileInfoProp_);
    expandFileInfoProp_->setValue(st);
    fileLabel_->setCompact(!st);
}

//------------------------------
// NodeObserver notofocations
//------------------------------

void OutputItemWidget::nodeChanged(const VNode* n, const std::vector<ecf::Aspect::Type>& aspect)
{
    //Changes in the nodes
    for(auto it : aspect)
    {
        if(it == ecf::Aspect::STATE || it == ecf::Aspect::DEFSTATUS ||
            it == ecf::Aspect::SUSPENDED)
        {
            if(submittedWarning_) {
               loadCurrentJobout();
            }
            else if(info_ && info_->node() == n && info_->node()->isSubmitted())
            {
                if (isJoboutLoaded()) {
                    loadCurrentJobout();
                }
            }
            return;
        }
    }
}

static InfoPanelItemMaker<OutputItemWidget> maker1("output");
