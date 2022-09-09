//============================================================================
// Copyright 2009- ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include <stdexcept>
#include "LogLoadWidget.hpp"
#include "LogLoadView.hpp"

#include "File_r.hpp"
#include "File.hpp"
#include "FileInfoLabel.hpp"
#include "LogModel.hpp"
#include "NodePath.hpp"
#include "ServerHandler.hpp"
#include "Str.hpp"
#include "SuiteFilter.hpp"
#include "TextFormat.hpp"
#include "TimelinePreLoadDialog.hpp"
#include "UiLog.hpp"
#include "UIDebug.hpp"
#include "ViewerUtil.hpp"
#include "VConfig.hpp"
#include "VFileInfo.hpp"
#include "VFileTransfer.hpp"
#include "VSettings.hpp"

#include <QtGlobal>
#include <QDateTime>
#include <QElapsedTimer>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QSortFilterProxyModel>
#include <QSplitter>
#include <QTableView>
#include <QStackedWidget>
#include <QTextBrowser>
#include <QTimer>
#include <QToolBox>
#include <QToolButton>
#include <QTreeView>
#include <QVBoxLayout>

#include "ui_LogLoadWidget.h"

#define UI_LOGLOADWIDGET_DEBUG_

//=======================================================
//
// LogLoadWidget
//
//=======================================================

LogLoadWidget::LogLoadWidget(QWidget *parent) :
    ui_(new Ui::LogLoadWidget)
{
    ui_->setupUi(this);

    //message label
    ui_->messageLabel->hide();

    connect(ui_->messageLabel,SIGNAL(loadStoppedByButton()),
            this,SLOT(slotCancelFileTransfer()));

    //Chart views
    viewHandler_=new LogRequestViewHandler(this);
    for(int i=0; i < viewHandler_->tabItems().count(); i++)
    {
        ui_->viewTab->addTab(viewHandler_->tabItems()[i]," h");
    }

    connect(viewHandler_->data(),SIGNAL(loadProgress(size_t,size_t)),
            this,SLOT(slotLogLoadProgress(size_t,size_t)));

    ui_->viewTab->setTabText(0,tr("Total charts"));
    ui_->viewTab->setTabText(1,tr("Other charts"));
    ui_->viewTab->setTabText(2,tr("Tables"));

    ui_->viewTab->setCurrentIndex(0);

    // TODO: the  period selection widgets are yet to be fully implemented.
    // At the moment, we just hide them and show the current period in a label.
    ui_->startTe->hide();
    ui_->endTe->hide();
    ui_->startLabel->hide();
    ui_->endLabel->hide();
    ui_->viewTab->setCornerWidget(ui_->cornerW);
    ui_->cornerHolderW->setVisible(false);

    ui_->timeLabel->setProperty("fileInfo","1");
    ui_->timeLabel->setMargin(2);
    ui_->timeLabel->setAlignment(Qt::AlignLeft|Qt::AlignVCenter);
    ui_->timeLabel->setAutoFillBackground(true);
    ui_->timeLabel->setTextInteractionFlags(Qt::LinksAccessibleByMouse|Qt::TextSelectableByKeyboard|Qt::TextSelectableByMouse);

    connect(ui_->showFullTb,SIGNAL(clicked()),
            viewHandler_,SLOT(showFullRange()));

    //Temporal resolution combo box
    ui_->resCombo->addItem("seconds",0);
    ui_->resCombo->addItem("minutes",1);
    ui_->resCombo->addItem("hours",2);

    connect(ui_->resCombo,SIGNAL(currentIndexChanged(int)),
            this,SLOT(resolutionChanged(int)));

    //Log contents
    logModel_=new LogModel(this);

    ui_->logView->setProperty("log","1");
    ui_->logView->setProperty("log","1");
    ui_->logView->setRootIsDecorated(false);
    ui_->logView->setLogModel(logModel_);
    ui_->logView->setUniformRowHeights(true);
    ui_->logView->setAlternatingRowColors(false);
    ui_->logView->setItemDelegate(new LogDelegate(this));
    ui_->logView->setContextMenuPolicy(Qt::ActionsContextMenu);

    //make the horizontal scrollbar work
    ui_->logView->header()->setStretchLastSection(false);
    ui_->logView->header()->setSectionResizeMode(QHeaderView::ResizeToContents);

    ui_->showLogTb->setChecked(false);
    ui_->logView->hide();
    connect(ui_->showLogTb, SIGNAL(clicked(bool)),
            this, SLOT(showLogView(bool)));

    //Define context menu
    //ui_->logView->addAction(actionCopyEntry_);
    //ui_->logView->addAction(actionCopyRow_);

    //Connect the views to the log model
    connect(viewHandler_,SIGNAL(timeRangeChanged(qint64,qint64)),
            logModel_,SLOT(setPeriod(qint64,qint64)));

    connect(viewHandler_,SIGNAL(timeRangeChanged(qint64,qint64)),
            this,SLOT(periodChanged(qint64,qint64)));

    connect(viewHandler_, SIGNAL(timeRangeHighlighted(qint64,qint64,qint64)),
            logModel_,SLOT(setHighlightPeriod(qint64,qint64,qint64)));

    connect(viewHandler_,SIGNAL(timeRangeReset()),
            logModel_,SLOT(resetPeriod()));

    connect(viewHandler_,SIGNAL(timeRangeReset()),
            this,SLOT(periodWasReset()));

    // log mode
    ui_->logModeCombo->addItem("Current log","latest");
    ui_->logModeCombo->addItem("Archived log","archive");
    ui_->logModeCombo->setCurrentIndex(0);
    ui_->loadFileTb->hide();

    // signal-slot
    connect(ui_->logModeCombo,SIGNAL(currentIndexChanged(int)),
            this,SLOT(slotLogMode(int)));

    connect(ui_->reloadTb,SIGNAL(clicked()),
            this,SLOT(slotReload()));

    connect(ui_->loadFileTb,SIGNAL(clicked()),
            this,SLOT(slotLoadCustomFile()));

    // the icon for this button changes according to state
    ui_->expandFileInfoTb->setIcon(ViewerUtil::makeExpandIcon(false));
    ui_->expandFileInfoTb->setMaximumSize(QSize(16, 16));
    expandFileInfoProp_ = VConfig::instance()->find("panel.timeline.expandFileInfo");
    Q_ASSERT(expandFileInfoProp_);
    bool expandSt = expandFileInfoProp_->value().toBool();
    ui_->expandFileInfoTb->setChecked(expandSt);
    connect(ui_->expandFileInfoTb,SIGNAL(clicked(bool)),
            this,SLOT(slotExpandFileInfo(bool)));
    slotExpandFileInfo(expandSt);
}

LogLoadWidget::~LogLoadWidget()
{
}

//void LogLoadWidget::initSplitter()
//{
//    if (!splitterInited_ && width() > 100) {
//        splitterInited_ = true;
//        if (splitterSavedState_.isEmpty()) {
//            auto w = width();
//            Q_ASSERT(ui_->splitter->count() == 3);
//            QList<int> sizes;
//            sizes << 300*1000/w << 600*1000/w;
//            sizes << w-(sizes[0]+sizes[1]);
//            ui_->splitter->setSizes(sizes);
//        } else {
//            ui_->splitter->restoreState(splitterSavedState_);
//        }
//    }
//}

void LogLoadWidget::clear()
{
    beingCleared_=true;
    if(fileTransfer_)
    {
        fileTransfer_->stopTransfer(true);
        ui_->messageLabel->stopLoadLabel();
    }

    ui_->messageLabel->clear();
    ui_->messageLabel->hide();

    ui_->logInfoLabel->clearIt();

    viewHandler_->clear();
    logModel_->clearData();
    logFile_.clear();
    serverName_.clear();
    host_.clear();
    port_.clear();
    suites_.clear();
    remoteUid_.clear();
    localLog_=true;
    logLoaded_=false;
    logTransferred_=false;
    transferredAt_=QDateTime();

    //prevState_.valid=false;
    tmpLogFile_.reset();
//    ui_->startTe->clear();
//    ui_->endTe->clear();
    ui_->timeLabel->clear();

    ui_->logModeCombo->setCurrentIndex(0);
    logMode_ = LatestMode;
    ui_->reloadTb->show();
    ui_->loadFileTb->hide();
    archiveLogList_.clear();
    ui_->logModeCombo->hide();

    setAllVisible(false);
    beingCleared_=false;
}

void LogLoadWidget::clearData(bool usePrevState)
{
    beingCleared_=true;
    if(fileTransfer_)
    {
        fileTransfer_->stopTransfer(true);
        ui_->messageLabel->stopLoadLabel();
    }

    ui_->messageLabel->clear();
    ui_->messageLabel->hide();
    ui_->logInfoLabel->clearIt();

    viewHandler_->clear();
    logModel_->clearData();

    localLog_=true;
    logLoaded_=false;
    logTransferred_=false;
    transferredAt_=QDateTime();

    if (!usePrevState)
    {
        prevState_.valid=false;
//        ui_->startTe->clear();
//        ui_->endTe->clear();
        ui_->timeLabel->clear();
    }

    tmpLogFile_.reset();
    setAllVisible(false);
    beingCleared_=false;
}


void LogLoadWidget::updateInfoLabel(bool showDetails)
{
    QString txt, compactTxt;

    auto data = viewHandler_->data();

    if(logMode_ == LatestMode)
    {
        txt= FileInfoLabel::formatKwPair("Log file", logFile_) +
             FileInfoLabel::formatKwPair(" Server", serverName_) +
             FileInfoLabel::formatKwPair(" Host", host_) +
             FileInfoLabel::formatKwPair(" Port", port_);

        if(showDetails)
        {
            if(data->loadStatus() == LogLoadData::LoadDone)
            {
                if(localLog_)
                {
                    VFileInfo fi(logFile_);
                    txt+=FileInfoLabel::formatKwPair(" Size", fi.formatSize());
                }
                else if (tmpLogFile_)
                {
                    VFileInfo fi(QString::fromStdString(tmpLogFile_->path()));
                    txt+=FileInfoLabel::formatKwPair(" Size", fi.formatSize());
                }
            }

            txt+=FileInfoLabel::formatKey(" Source");

            //fetch method and time
            if(localLog_)
            {
                txt+=" read from disk ";
                if(data->loadedAt().isValid())
                    txt+=FileInfoLabel::formatHighlight(" at ") + FileInfoLabel::formatDate(data->loadedAt());
            }
            else
            {
                if(logTransferred_)
                {
                    txt+=" fetched from remote host ";
                }
                else
                {
                    txt+=" fetch failed from remote host ";
                }
                if(transferredAt_.isValid())
                    txt+=FileInfoLabel::formatHighlight(" at ") + FileInfoLabel::formatDate(transferredAt_);
            }

            if(data->loadStatus() == LogLoadData::LoadDone)
            {
                QColor warnCol(218,142,18);

                QString warnVerb;
                if(localLog_ && !data->isFullRead())
                {
                    warnVerb="parsed";
                }
                else if(tmpLogFile_)
                {
                    QFileInfo fi(QString::fromStdString(tmpLogFile_->path()));
                    if(maxReadSize_ > 0 && static_cast<size_t>(fi.size()) == maxReadSize_)
                    {
                        warnVerb="fetched";
                    }
                }

                if(!warnVerb.isEmpty())
                {
                    txt+=Viewer::formatItalicText(" (" + warnVerb + " last " + QString::number(maxReadSize_/(1024*1024)) +
                                        " MB of file - maximum size reached)",warnCol);
                }
            }
        }

        VFileInfo fInfo(logFile_);
        compactTxt = FileInfoLabel::formatKwPair("File", fInfo.fileName());
    }

    //archived
    else
    {
        if(archiveLogList_.loadableCount() == 1)
        {
           txt= FileInfoLabel::formatKwPair("Log file",archiveLogList_.firstLoadablePath());
        }
        else if(archiveLogList_.loadableCount() > 1)
        {
            txt= FileInfoLabel::formatKwPair("Log files", "multiple ["+
                    QString::number(archiveLogList_.loadableCount())  + "]");
        }

        txt+=FileInfoLabel::formatKwPair(" Server", serverName_) +
             FileInfoLabel::formatKwPair(" Host", host_) +
             FileInfoLabel::formatKwPair(" Port", port_);

        if(showDetails)
        {
            if(data->loadStatus() == LogLoadData::LoadDone)
            {
                if(archiveLogList_.loadableCount() == 1)
                    txt+=FileInfoLabel::formatKwPair(" Size", VFileInfo::formatSize(archiveLogList_.totalSize()));
                else if (archiveLogList_.loadableCount() > 1)
                    txt+=FileInfoLabel::formatKwPair(" Total size", VFileInfo::formatSize(archiveLogList_.totalSize()));
            }
            txt+=FileInfoLabel::formatKwPair(" Source", "read from disk ");
        }

        compactTxt = FileInfoLabel::formatKwPair("Log files", "multiple ["+
                           QString::number(archiveLogList_.loadableCount())  + "]");
    }

    ui_->logInfoLabel->update(txt, compactTxt);

    checkButtonState();
}

void LogLoadWidget::showLogView(bool b)
{
    if (b && ui_->resCombo->isVisible()) {
        ui_->logView->setVisible(true);
    } else {
        ui_->logView->setVisible(false);
    }
}

bool LogLoadWidget::shouldShowLog() const
{
    return ui_->showLogTb->isChecked();
}

void LogLoadWidget::setAllVisible(bool b)
{
    ui_->viewTab->setVisible(b);
    ui_->logView->setVisible(b && shouldShowLog());
    ui_->cornerW->setVisible(b);
}

void LogLoadWidget::slotLogMode(int)
{
    if(beingCleared_)
        return;

#if QT_VERSION >= QT_VERSION_CHECK(5, 2, 0)
    QString id=ui_->logModeCombo->currentData().toString();
#else
    QString id;
    if(ui_->logModeCombo->currentIndex() >=0)
        id=ui_->logModeCombo->itemData(ui_->logModeCombo->currentIndex()).toString();
#endif
    if(id == "latest")
        setLogMode(LatestMode);
    else if(id == "archive")
        setLogMode(ArchiveMode);
}

void LogLoadWidget::setLogMode(LogMode logMode)
{
    if(logMode_ == logMode)
        return;

    logMode_ = logMode;
    if (logMode_ == LatestMode)
    {
        ui_->reloadTb->show();
        ui_->loadFileTb->hide();
        archiveLogList_.clear();
        clearData(false);
        reloadLatest(false);
    }
    else if(logMode_ == ArchiveMode)
    {
        ui_->reloadTb->hide();
        ui_->loadFileTb->show();
        archiveLogList_.clear();
        clearData(false);
        updateInfoLabel();
        checkButtonState();
    }
    else
    {
        UI_ASSERT(0, "Invalid logMode=" << logMode);
    }
}

// The reload button was pressed in LatestMode
void LogLoadWidget::slotReload()
{
    Q_ASSERT(logMode_ == LatestMode);
    reloadLatest(true);
}

// Reload the logfile in current mode.
// canUsePrevState: false when we come form ArchiveMode or initial load,
//                  true otherwise
void LogLoadWidget::reloadLatest(bool usePrevState)
{
    Q_ASSERT(logMode_ == LatestMode);
    if(!serverName_.isEmpty())
    {
        std::vector<std::string> suites;
        //we get the latest state of the suites and the remote uid
        if(ServerHandler *sh=ServerHandler::find(serverName_.toStdString()))
        {
            remoteUid_ = sh->uidForServerLogTransfer();
            setMaxReadSize(sh->maxSizeForTimelineData());

            if(SuiteFilter* sf=sh->suiteFilter())
            {
                if(sf->isEnabled())
                    suites=sh->suiteFilter()->filter();
            }

            suites_ = suites;
        }

        loadLatest(usePrevState);
        checkButtonState();
    }
}

void LogLoadWidget::slotLoadCustomFile()
{
    Q_ASSERT(logMode_ == ArchiveMode);
    QStringList fileNames = QFileDialog::getOpenFileNames(this);
    if(fileNames.isEmpty())
        return;

    TimelineFileList archiveLogList = TimelineFileList(fileNames);
    TimelinePreLoadDialog dialog;
    dialog.setModal(true);
    dialog.init(archiveLogList);
    if(dialog.exec() == QDialog::Accepted)
    {
        archiveLogList_ = archiveLogList;
        loadArchive();
    }
}


// Initial load
void LogLoadWidget::initLoad(QString serverName, QString host, QString port, QString logFile,
                          const std::vector<std::string>& suites, QString remoteUid, int maxReadSize,
                          const std::string& currentNodePath, bool detached)
{
    if(logMode_ != LatestMode)
        return;

    // we need to be in a clean state
    clear();
    Q_ASSERT(logMode_ == LatestMode);

    setDetached(detached);

    // clear() hides it so we need to show it again
    ui_->logModeCombo->show();

    // these must be kept until we call clear()!!!!
    serverName_=serverName;
    host_=host;
    port_=port;
    logFile_=logFile;
    suites_=suites;
    remoteUid_=remoteUid;
    setMaxReadSize(maxReadSize);
    currentNodePath_=QString::fromStdString(currentNodePath);

    loadLatest(false);
}

void LogLoadWidget::loadLatest(bool usePrevState)
{
    Q_ASSERT(logMode_ == LatestMode);

    //if it is a reload we remember the current period
    if(usePrevState)
    {
        auto data = viewHandler_->data();
        prevState_.valid=true;
        prevState_.startDt=ui_->startTe->dateTime();
        prevState_.endDt=ui_->endTe->dateTime();
        prevState_.fullStart=(prevState_.startDt == data->startTime());
        prevState_.fullEnd=(prevState_.endDt == data->endTime());
        clearData(true);
    }
    else
    {
        //prevState_.valid=false;
        clearData(false);
    }

    if(logFile_.isEmpty())
       return;

    logLoaded_=false;
    logTransferred_=false;
    localLog_=true;

    // at this point the file info label can only show some basic information
    // we do not have the details
    updateInfoLabel(false);

    QFileInfo fInfo(logFile_);

    //try to get it over the network, ascynchronous - will call loadCore() in the end
    if(!fInfo.exists())
    {
        localLog_=false;
        //tmpLogFile_=VFile::createTmpFile(true); //will be deleted automatically
        ui_->messageLabel->showInfo("Fetching file from remote host ...");
        ui_->messageLabel->startLoadLabel(true);

        if(!fileTransfer_)
        {
            fileTransfer_=new VFileTransfer(this);

            connect(fileTransfer_,SIGNAL(transferFinished()),
                    this,SLOT(slotFileTransferFinished()));

            connect(fileTransfer_,SIGNAL(transferFailed(QString)),
                    this,SLOT(slotFileTransferFailed(QString)));

            connect(fileTransfer_,SIGNAL(stdOutputAvailable(QString)),
                    this,SLOT(slotFileTransferStdOutput(QString)));
        }

        if (VConfig::instance()->proxychainsUsed()) {
           fileTransfer_->transferLocalViaSocks(logFile_,VFileTransfer::LastBytes, maxReadSize_);
        } else {
            fileTransfer_->transfer(logFile_,host_,remoteUid_,VFileTransfer::LastBytes, maxReadSize_);
        }
    }
    // load local file
    else
    {
        loadCore(logFile_);
    }
}

// Load a single/multiple logfiles in ArchiveMode
void LogLoadWidget::loadArchive()
{
    Q_ASSERT(logMode_ == ArchiveMode);

    clearData(false);

    logLoaded_=false;
    logTransferred_=false;
    localLog_=true;

    ViewerUtil::setOverrideCursor(QCursor(Qt::WaitCursor));

    ui_->messageLabel->showInfo("Loading data from log file ...");

    bool loadDone=false;

    QElapsedTimer timer;
    timer.start();

    logModel_->beginLoadFromReader();

    //std::vector<std::string> suites;
    for(int i=0; i < archiveLogList_.items().count(); i++)
    {
        if(!archiveLogList_.items()[i].loadable_)
            continue;

        ui_->messageLabel->showInfo("Loading data from log file [" +
                                    QString::number(i+1) + "/" + QString::number(archiveLogList_.items().count()) +
                                    "] ...");

        ui_->messageLabel->startProgress(100);

        try
        {
            viewHandler_->loadMultiLogFile(archiveLogList_.items()[i].dataPath().toStdString(),suites_,
                                           i, (i == archiveLogList_.items().count()-1),
                                           logModel_->logData());

            loadDone=true;
        }

        catch(std::runtime_error e)
        {
            logModel_->endLoadFromReader();
            //logLoaded_=false;
            ui_->messageLabel->stopProgress();
            std::string errTxt(e.what());
        }

        UiLog().dbg() << "Logfile parsed: " << timer.elapsed()/1000 << "s";

        ui_->messageLabel->stopProgress();
    }

    logModel_->endLoadFromReader();
    viewHandler_->loadPostProc();

    ui_->messageLabel->hide();

    ViewerUtil::restoreOverrideCursor();

    if(!loadDone)
    {
        ui_->messageLabel->showError(QString::fromStdString("Could not parse any of the specified log files!"));
        viewHandler_->clear();
        logModel_->clearData();
        setAllVisible(false);
        updateInfoLabel();
        return;
    }

    logLoaded_=true;
    viewHandler_->data()->markAsLoadDone();
    setAllVisible(true);
    updateInfoLabel();

    initFromData();
}

void LogLoadWidget::slotFileTransferFinished()
{
    //we are not in a cleared state
    if(!beingCleared_)
    {
        tmpLogFile_ = fileTransfer_->result();
        fileTransfer_->clear();
        logTransferred_=true;
        ui_->messageLabel->stopLoadLabel();
        ui_->messageLabel->hide();
        ui_->messageLabel->update();
        if (tmpLogFile_) {
            loadCore(QString::fromStdString(tmpLogFile_->path()));
        }
    }
}

void LogLoadWidget::slotFileTransferFailed(QString err)
{
    if(!beingCleared_)
    {
        tmpLogFile_.reset();
        fileTransfer_->clear();
        logTransferred_=false;
        ui_->messageLabel->stopLoadLabel();
        logLoaded_=false;
        ui_->messageLabel->showError("Could not fetch log file from remote host! <br>" + err);
        viewHandler_->clear();
        logModel_->clearData();
        setAllVisible(false);
        updateInfoLabel();
    }
}

void LogLoadWidget::slotFileTransferStdOutput(QString msg)
{
    if(!msg.simplified().isEmpty())
    {
        ui_->messageLabel->appendInfo(msg);
    }
}


void LogLoadWidget::slotLogLoadProgress(size_t current,size_t total)
{
    int percent=100*current/total;
    if(percent >=0 && percent <=100)
        ui_->messageLabel->progress("",percent);

}

// notification from messagelabel: user cancelled the transfer
void LogLoadWidget::slotCancelFileTransfer()
{
#ifdef UI_LOGLOADWIDGET_DEBUG_
    UI_FUNCTION_LOG
    UiLog().dbg() <<  "fileTransfer_=" << fileTransfer_;
#endif
    if(fileTransfer_)
    {
        fileTransfer_->stopTransfer(true);
    }
}

void LogLoadWidget::loadCore(QString logFile)
{
    ViewerUtil::setOverrideCursor(QCursor(Qt::WaitCursor));
    VFileInfo fInfo(logFile);
    ui_->messageLabel->showInfo("Loading data from log file ... [size=" +
                                fInfo.formatSize() + "]");

    QElapsedTimer timer;
    timer.start();

    ui_->messageLabel->startProgress(100);

    try
    {
        logModel_->beginLoadFromReader();
        viewHandler_->load(logFile.toStdString(),maxReadSize_,suites_,
                           logModel_->logData());
        logModel_->endLoadFromReader();
    }
    catch(std::runtime_error e)
    {
        logModel_->endLoadFromReader();

        logLoaded_=false;
        ui_->messageLabel->stopProgress();

        std::string errTxt(e.what());

        QFileInfo fInfo(logFile);
        if(!fInfo.exists())
        {
            errTxt+=" The specified log file <b>does not exist</b> on disk!";
        }
        else if(!fInfo.isReadable())
        {
            errTxt+=" The specified log file is <b>not readable</b>!";
        }
        else if(!fInfo.isFile())
        {
            errTxt+=" The specified log file is <b>not a file</b>!";
        }

        ui_->messageLabel->showError(QString::fromStdString(errTxt));
        viewHandler_->clear();
        logModel_->clearData();
        setAllVisible(false);
        updateInfoLabel();
        ViewerUtil::restoreOverrideCursor();
        return;
    }

    UiLog().dbg() << "Logfile parsed: " << timer.elapsed()/1000 << "s";

    ui_->messageLabel->stopProgress();
    ui_->messageLabel->hide();
    logLoaded_=true;
    setAllVisible(true);
    updateInfoLabel();

    ViewerUtil::restoreOverrideCursor();

    initFromData();
}

void LogLoadWidget::initFromData()
{
    auto data = viewHandler_->data();
    QDateTime startTime=data->startTime();
    QDateTime endTime=data->endTime();
    ui_->startTe->setMinimumDateTime(startTime);
    ui_->startTe->setMaximumDateTime(endTime);

    //TODO: implement period initialisation based on prev state
    bool usePrevState = false;

    //try the set the previously used interval - for reload only
    if(usePrevState && prevState_.valid)
    {
        if(prevState_.startDt <= data->startTime() || prevState_.fullStart)
            ui_->startTe->setDateTime(data->startTime());
        else if(prevState_.startDt < data->endTime())
            ui_->startTe->setDateTime(prevState_.startDt);
    }
    else
    {
        ui_->startTe->setDateTime(data->startTime());
    }
    ui_->endTe->setMinimumDateTime(startTime);
    ui_->endTe->setMaximumDateTime(endTime);

    if(usePrevState && prevState_.valid)
    {
        if(prevState_.endDt >= data->endTime() || prevState_.fullEnd)
            ui_->endTe->setDateTime(data->endTime());
        else if(prevState_.endDt > data->startTime())
            ui_->endTe->setDateTime(prevState_.endDt);
    }
    else
    {
        ui_->endTe->setDateTime(data->endTime());
    }

    updateTimeLabel(startTime, endTime);

    checkButtonState();
}

void LogLoadWidget::setDetached(bool d)
{
    detached_=d;
}

void LogLoadWidget::checkButtonState()
{
     auto data = viewHandler_->data();
     bool fromStart=(ui_->startTe->dateTime() == data->startTime());
     bool toEnd=(ui_->endTe->dateTime() == data->endTime());

//     ui_->startTb->setEnabled(!fromStart);
//     ui_->endTb->setEnabled(!toEnd);
     ui_->showFullTb->setEnabled(!fromStart || !toEnd);
}

void LogLoadWidget::setMaxReadSize(int maxReadSizeInMb)
{
    if (maxReadSizeInMb <= 0)
        maxReadSize_ = 0;
    else
        maxReadSize_ = static_cast<size_t>(maxReadSizeInMb)*1024*1024;
}

void LogLoadWidget::periodChanged(qint64 start,qint64 end)
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 2, 0)
    QDateTime startDt=QDateTime::fromMSecsSinceEpoch(start,Qt::UTC);
    QDateTime endDt=QDateTime::fromMSecsSinceEpoch(end,Qt::UTC);
#else
    QDateTime startDt=QDateTime::fromMSecsSinceEpoch(start).toUTC();
    QDateTime endDt=QDateTime::fromMSecsSinceEpoch(end).toUTC();
#endif
    ui_->startTe->setDateTime(startDt);
    ui_->endTe->setDateTime(endDt);
    updateTimeLabel(startDt, endDt);
}

void LogLoadWidget::periodWasReset()
{
    QDateTime startDt=viewHandler_->data()->startTime();
    QDateTime endDt=viewHandler_->data()->endTime();
    ui_->startTe->setDateTime(startDt);
    ui_->endTe->setDateTime(endDt);
    updateTimeLabel(startDt, endDt);
    checkButtonState();
}

void LogLoadWidget::updateTimeLabel(QDateTime startDt, QDateTime endDt)
{
    QColor col(39,49,101);
    QString txt = "start: " +
            Viewer::formatBoldText(startDt.toString("hh:mm:ss yyyy-MM-dd"), col) +
            " end: " +
            Viewer::formatBoldText(endDt.toString("hh:mm:ss yyyy-MM-dd"), col);
    ui_->timeLabel->setText(txt);
}


void LogLoadWidget::resolutionChanged(int)
{
    int idx=ui_->resCombo->currentIndex();
    if(idx == 0)
        viewHandler_->setResolution(LogLoadData::SecondResolution);
    else if(idx == 1)
        viewHandler_->setResolution(LogLoadData::MinuteResolution);
    else if(idx == 2)
        viewHandler_->setResolution(LogLoadData::HourResolution);
}

//-------------------------
// File info label
//-------------------------

void LogLoadWidget::slotExpandFileInfo(bool st)
{
    Q_ASSERT(expandFileInfoProp_);
    expandFileInfoProp_->setValue(st);
    ui_->logInfoLabel->setCompact(!st);
}


void LogLoadWidget::writeSettings(VComboSettings* vs)
{
//    int cbIdx=ui_->resCombo->currentIndex();
//    if (cbIdx) {
//        vs->put("plotResolution",
//                ui_->resCombo->itemData(cbIdx).toString().toStdString());
//    }
//    vs->putQs("splitter",ui_->splitter->saveState());
    viewHandler_->writeSettings(vs);
}

void LogLoadWidget::readSettings(VComboSettings* vs)
{
    //sort mode
//    QString resMode=QString::fromStdString(vs->get<std::string>("plotResolution", std::string()));
//    ViewerUtil::initComboBoxByData(resMode,ui_->resCombo);

//    splitterSavedState_ = vs->getQs("splitter").toByteArray();
    viewHandler_->readSettings(vs);
}
