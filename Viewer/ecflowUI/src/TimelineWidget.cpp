//============================================================================
// Copyright 2009-2018 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "TimelineWidget.hpp"

#include <QtGlobal>
#include <QFileInfo>
#include <QTime>
#include <QButtonGroup>

#include "IconProvider.hpp"
#include "FileInfoLabel.hpp"
#include "MainWindow.hpp"
#include "ServerHandler.hpp"
#include "TimelineData.hpp"
#include "TimelineModel.hpp"
#include "TimelineView.hpp"
#include "TextFormat.hpp"
#include "UiLog.hpp"
#include "ViewerUtil.hpp"
#include "VFileInfo.hpp"
#include "VFileTransfer.hpp"
#include "VNode.hpp"
#include "VSettings.hpp"

#include "ui_TimelineWidget.h"

//=======================================================
//
// TimelineWidget
//
//=======================================================

TimelineWidget::TimelineWidget(QWidget *parent) :
    ui_(new Ui::TimelineWidget),
    maxReadSize_(100*1024*1024),
    data_(0),
    ignoreTimeEdited_(false),
    beingCleared_(false),
    typesDetermined_(false),
    localLog_(true),
    logLoaded_(false),
    logTransferred_(false),
    fileTransfer_(0)
{
    ui_->setupUi(this);

    //message label
    ui_->messageLabel->hide();

    data_=new TimelineData(this);

    connect(data_,SIGNAL(loadProgress(size_t,size_t)),
            this,SLOT(slotLogLoadProgress(size_t,size_t)));

    //the models
    model_=new TimelineModel(this);
    sortModel_=new TimelineSortModel(model_,this);

    model_->setData(data_);

    view_=new TimelineView(sortModel_,this);

    ui_->viewHolderLayout->addWidget(view_);

    //ui_->view->setModel(model_);

    //make the horizontal scrollbar work
    //ui_->logView->header()->setStretchLastSection(false);
    //ui_->logView->header()->setSectionResizeMode(QHeaderView::ResizeToContents);

    //Define context menu
    //ui_->logView->addAction(actionCopyEntry_);
    //ui_->logView->addAction(actionCopyRow_);

    //logInfo label
    ui_->logInfoLabel->setProperty("fileInfo","1");
    ui_->logInfoLabel->setWordWrap(true);
    ui_->logInfoLabel->setMargin(2);
    ui_->logInfoLabel->setAlignment(Qt::AlignLeft|Qt::AlignVCenter);
    ui_->logInfoLabel->setAutoFillBackground(true);
    ui_->logInfoLabel->setFrameShape(QFrame::StyledPanel);
    ui_->logInfoLabel->setTextInteractionFlags(Qt::LinksAccessibleByMouse|Qt::TextSelectableByKeyboard|Qt::TextSelectableByMouse);

    ui_->modeCombo->addItem("Timeline","timeline");
    ui_->modeCombo->addItem("Duration","duration");
    ui_->modeCombo->setCurrentIndex(0);

#if QT_VERSION >= QT_VERSION_CHECK(4, 7, 0)
    ui_->pathFilterLe->setPlaceholderText(tr("Filter"));
#endif

#if QT_VERSION >= QT_VERSION_CHECK(5, 2, 0)
    ui_->pathFilterLe->setClearButtonEnabled(true);
#endif

    ui_->sortCombo->addItem("Sort by path","path");
    ui_->sortCombo->addItem("Sort by time","time");
    ui_->sortCombo->setCurrentIndex(0);

    QButtonGroup *sortGr = new QButtonGroup(this);
    sortGr->addButton(ui_->sortUpTb,0);
    sortGr->addButton(ui_->sortDownTb,1);
    sortGr->setExclusive(true);
    ui_->sortUpTb->setChecked(true);

    ui_->showChangedTb->setChecked(true);

    ui_->fromTimeEdit->setDisplayFormat("hh:mm:ss dd-MMM-2018");
    ui_->toTimeEdit->setDisplayFormat("hh:mm:ss dd-MMM-2018");

    ui_->zoomInTb->setDefaultAction(ui_->actionZoomIn);
    ui_->zoomOutTb->setDefaultAction(ui_->actionZoomOut);

    view_->setZoomActions(ui_->actionZoomIn,ui_->actionZoomOut);

    connect(ui_->modeCombo,SIGNAL(currentIndexChanged(int)),
            this,SLOT(slotViewMode(int)));

    connect(ui_->pathFilterLe,SIGNAL(textChanged(QString)),
            this,SLOT(slotPathFilter(QString)));

    connect(ui_->taskOnlyTb,SIGNAL(clicked(bool)),
            this,SLOT(slotTaskOnly(bool)));

    connect(ui_->sortCombo,SIGNAL(currentIndexChanged(int)),
            this,SLOT(slotSortMode(int)));

    connect(sortGr,SIGNAL(buttonClicked(int)),
            this,SLOT(slotSortOrderChanged(int)));

    connect(ui_->showChangedTb,SIGNAL(clicked(bool)),
            this,SLOT(slotShowChanged(bool)));

    connect(ui_->fromTimeEdit,SIGNAL(dateTimeChanged(QDateTime)),
            this,SLOT(slotStartChanged(QDateTime)));

    connect(ui_->startTb,SIGNAL(clicked()),
            this,SLOT(slotResetStart()));

    connect(ui_->endTb,SIGNAL(clicked()),
            this,SLOT(slotResetEnd()));

    connect(ui_->reloadTb,SIGNAL(clicked()),
            this,SLOT(slotReload()));

    connect(ui_->wholePeriodTb,SIGNAL(clicked()),
            this,SLOT(slotWholePeriod()));

    connect(view_,SIGNAL(periodSelected(QDateTime,QDateTime)),
            this,SLOT(slotPeriodSelectedInView(QDateTime,QDateTime)));

    connect(view_,SIGNAL(periodBeingZoomed(QDateTime,QDateTime)),
            this,SLOT(slotPeriodBeingZoomedInView(QDateTime,QDateTime)));

    connect(view_,SIGNAL(lookupRequested(QString)),
            this,SLOT(slotLookup(QString)));

    connect(view_,SIGNAL(copyPathRequested(QString)),
            this,SLOT(slotCopyPath(QString)));

    //forced init
    slotSortMode(0);
    slotSortOrderChanged(0);
    slotShowChanged(true);
}

TimelineWidget::~TimelineWidget()
{
    if(data_)
        delete data_;
}

void TimelineWidget::clear(bool inReload)
{   
    beingCleared_=true;
    if(fileTransfer_)
    {
        fileTransfer_->stopTransfer();
        ui_->messageLabel->stopLoadLabel();
    }

    ui_->messageLabel->clear();
    ui_->messageLabel->hide();

    ui_->logInfoLabel->setText(QString());
    //viewHandler_->clear();

    model_->clearData();
    data_->clear();
    logFile_.clear();
    serverName_.clear();
    host_.clear();
    port_.clear();
    suites_.clear();
    typesDetermined_=false;
    localLog_=true;
    logLoaded_=false;
    logTransferred_=false;
    transferredAt_=QDateTime();

    if(!inReload)
    {
        ui_->pathFilterLe->clear();
        prevState_.valid=false;
    }

    tmpLogFile_.reset();
    //ui_->startTe->clear();
    //ui_->endTe->clear();

    setAllVisible(false);

    beingCleared_=false;
}

void TimelineWidget::updateInfoLabel(bool showDetails)
{
    QColor col(39,49,101);
    QString txt=Viewer::formatBoldText("Log file: ",col) + logFile_;
    txt+=Viewer::formatBoldText(" Server: ",col) + serverName_ +
         Viewer::formatBoldText(" Host: ",col) + host_ +
         Viewer::formatBoldText(" Port: ",col) + port_;

    if(showDetails)
    {
        if(data_->loadStatus() == TimelineData::LoadDone)
        {
            if(localLog_)
            {
                VFileInfo fi(logFile_);
                txt+=Viewer::formatBoldText(" Size: ",col) + fi.formatSize();
            }
            else
            {
                 VFileInfo fi(QString::fromStdString(tmpLogFile_->path()));
                 if(fi.size() < static_cast<qint64>(maxReadSize_))
                 {
                     txt+=Viewer::formatBoldText(" Size: ",col) + fi.formatSize();
                 }
            }
        }

        txt+=Viewer::formatBoldText(" Source: ",col);

        //fetch method and time
        if(localLog_)
        {
            txt+=" read from disk ";
            if(data_->loadedAt().isValid())
                txt+=Viewer::formatBoldText(" at ",col) + FileInfoLabel::formatDate(data_->loadedAt());
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
                txt+=Viewer::formatBoldText(" at ",col) + FileInfoLabel::formatDate(transferredAt_);
        }

        if(data_->loadStatus() == TimelineData::LoadDone)
        {
            QColor warnCol(218,142,18);

            QString warnVerb;
            if(localLog_ && !data_->isFullRead())
            {
                warnVerb="parsed";
            }
            else if(tmpLogFile_)
            {
                QFileInfo fi(QString::fromStdString(tmpLogFile_->path()));
                if(static_cast<size_t>(fi.size()) == maxReadSize_)
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

    ui_->logInfoLabel->setText(txt);

    checkButtonState();
}

void TimelineWidget::setAllVisible(bool b)
{
    ui_->viewControl->setVisible(b);
    view_->setVisible(b);
}

void TimelineWidget::slotReload()
{
    if(!serverName_.isEmpty())
    {
        load(serverName_, host_, port_, logFile_,suites_);
        checkButtonState();
    }
}

void TimelineWidget::slotPeriodSelectedInView(QDateTime start,QDateTime end)
{
    Q_ASSERT(data_);
    if(start >= data_->qStartTime() && end <= data_->qEndTime())
    {
        ignoreTimeEdited_=true;
        ui_->fromTimeEdit->setDateTime(start);
        ui_->toTimeEdit->setDateTime(end);
        ignoreTimeEdited_=false;
    }
    checkButtonState();
}

void TimelineWidget::slotPeriodBeingZoomedInView(QDateTime start,QDateTime end)
{
    Q_ASSERT(data_);
    if(start >= data_->qStartTime() && end <= data_->qEndTime())
    {
        ignoreTimeEdited_=true;
        ui_->fromTimeEdit->setDateTime(start);
        ui_->toTimeEdit->setDateTime(end);
        ignoreTimeEdited_=false;
    }
    //checkButtonState();
}

void TimelineWidget::slotViewMode(int)
{
    QString id=ui_->modeCombo->currentData().toString();
    if(id == "timeline")
        view_->setViewMode(TimelineView::TimelineMode);
    else if (id == "duration")
        view_->setViewMode(TimelineView::DurationMode);
}


void TimelineWidget::slotTaskOnly(bool taskFilter)
{
    if(taskFilter)
    {
        determineNodeTypes();
    }

    sortModel_->setTaskFilter(taskFilter);
}

void TimelineWidget::slotPathFilter(QString pattern)
{
    sortModel_->setPathFilter(pattern);
}

void TimelineWidget::slotSortMode(int)
{
    int idx=ui_->sortCombo->currentIndex();
    if(idx == 0)
        sortModel_->setSortMode(TimelineSortModel::PathSortMode);
    else if (idx == 1)
        sortModel_->setSortMode(TimelineSortModel::TimeSortMode);
}

void TimelineWidget::slotSortOrderChanged(int)
{
    bool ascending=ui_->sortUpTb->isChecked();
    sortModel_->setSortDirection(ascending);
}

void TimelineWidget::slotShowChanged(bool st)
{
    sortModel_->setShowChangedOnly(st);
}

void TimelineWidget::slotStartChanged(const QDateTime& dt)
{
    if(!ignoreTimeEdited_)
    {        
        view_->setStartDate(dt);
    }
    checkButtonState();
}

void TimelineWidget::slotEndChanged(const QDateTime& dt)
{
    if(!ignoreTimeEdited_)
    {
        view_->setEndDate(dt);
    }
    checkButtonState();
}

void TimelineWidget::slotResetStart()
{
    ui_->fromTimeEdit->setDateTime(data_->qStartTime());
    checkButtonState();
}

void TimelineWidget::slotResetEnd()
{
    ui_->toTimeEdit->setDateTime(data_->qEndTime());
    checkButtonState();
}

void TimelineWidget::slotWholePeriod()
{
    Q_ASSERT(data_);
    ignoreTimeEdited_=true;
    ui_->fromTimeEdit->setDateTime(data_->qStartTime());
    ui_->toTimeEdit->setDateTime(data_->qEndTime());
    ignoreTimeEdited_=false;
    view_->setPeriod(data_->qStartTime(),data_->qEndTime());
    checkButtonState();
}

void TimelineWidget::slotLookup(QString nodePath)
{
    if(ServerHandler *sh=ServerHandler::find(serverName_.toStdString()))
    {
        VInfo_ptr ni=VInfo::createFromPath(sh,nodePath.toStdString());
        if(ni)
        {
            MainWindow::lookUpInTree(ni);
        }
    }
}

void TimelineWidget::slotCopyPath(QString nodePath)
{
    ViewerUtil::toClipboard(serverName_ + ":/" + nodePath);
}

void TimelineWidget::selectPathInView(const std::string& p)
{
    QModelIndex idx=sortModel_->mapToSource(view_->currentIndex());
    if(idx.isValid() && idx.row() < static_cast<int>(data_->size()))
    {
        if(data_->items()[idx.row()].path() == p)
            return;
    }

    size_t pos=0;
    if(data_->indexOfItem(p,pos))
    {
        QModelIndex idx=sortModel_->mapFromSource(model_->index(pos,0));
        view_->setCurrentIndex(idx);
    }
}

void TimelineWidget::checkButtonState()
{
     bool fromStart=(ui_->fromTimeEdit->dateTime() == data_->qStartTime());
     bool toEnd=(ui_->toTimeEdit->dateTime() == data_->qEndTime());

     ui_->startTb->setEnabled(!fromStart);
     ui_->endTb->setEnabled(!toEnd);
     ui_->wholePeriodTb->setEnabled(!fromStart || !toEnd);
}

void TimelineWidget::load(QString logFile)
{
    load("","","",logFile,suites_);
}

void TimelineWidget::load(QString serverName, QString host, QString port, QString logFile,
                          const std::vector<std::string>& suites)
{
    //if it is a reload we remember the current period
    if(!serverName.isEmpty() && serverName == serverName_ && host == host_ && port == port_)
    {
        prevState_.valid=true;
        prevState_.startDt=ui_->fromTimeEdit->dateTime();
        prevState_.endDt=ui_->toTimeEdit->dateTime();
        prevState_.fullStart=(prevState_.startDt == data_->qStartTime());
        prevState_.fullEnd=(prevState_.endDt == data_->qEndTime());

        clear(true);
    }
    else
    {
        prevState_.valid=false;
        clear();
    }

    if(logFile.isEmpty())
       return;

    serverName_=serverName;
    host_=host;
    port_=port;
    logFile_=logFile;
    suites_=suites;
    logLoaded_=false;
    logTransferred_=false;
    localLog_=true;

    updateInfoLabel(false);

    QFileInfo fInfo(logFile);

    //try to get it over the network, ascynchronous
    if(!fInfo.exists())
    {
        localLog_=false;
        tmpLogFile_=VFile::createTmpFile(true); //will be deleted automatically
        ui_->messageLabel->showInfo("Fetching file from remote host ... <br>");
        ui_->messageLabel->startLoadLabel();

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

        fileTransfer_->transfer(logFile_,host_,QString::fromStdString(tmpLogFile_->path()),maxReadSize_);
    }
    else
    {
        loadCore(logFile_);
    }
}

void TimelineWidget::slotFileTransferFinished()
{ 
    //we are not in a cleared state
    if(!beingCleared_)
    {
        logTransferred_=true;
        ui_->messageLabel->stopLoadLabel();
        ui_->messageLabel->hide();
        ui_->messageLabel->update();
        loadCore(QString::fromStdString(tmpLogFile_->path()));
    }
}

void TimelineWidget::slotFileTransferFailed(QString err)
{
    if(!beingCleared_)
    {
        logTransferred_=false;
        ui_->messageLabel->stopLoadLabel();
        logLoaded_=false;
        ui_->messageLabel->showError("Could not fetch log file from remote host! <br>" + err);
        data_->clear();
        setAllVisible(false);
        updateInfoLabel();
    }
}

void TimelineWidget::slotFileTransferStdOutput(QString msg)
{
    ui_->messageLabel->showInfo("Fetching file form remote host ... <br>" + msg);
}


void TimelineWidget::slotLogLoadProgress(size_t current,size_t total)
{
    int percent=100*current/total;
    if(percent >=0 && percent <=100)
        ui_->messageLabel->progress("",percent);

}

void TimelineWidget::loadCore(QString logFile)
{
    ViewerUtil::setOverrideCursor(QCursor(Qt::WaitCursor));

    ui_->messageLabel->showInfo("Loading timeline data from log file ...");

    QTime timer;
    timer.start();

    ui_->messageLabel->startProgress(100);

    try
    {
        data_->loadLogFile(logFile.toStdString(),maxReadSize_,suites_);
    }
    catch(std::runtime_error e)
    {
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
        data_->clear();
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

    //set the period

    ignoreTimeEdited_=true;

    ui_->fromTimeEdit->setMinimumDateTime(data_->qStartTime());
    ui_->fromTimeEdit->setMaximumDateTime(data_->qEndTime());

    //try the set the previously used interval - for reload only
    if(prevState_.valid)
    {
        if(prevState_.startDt <= data_->qStartTime() || prevState_.fullStart)
            ui_->fromTimeEdit->setDateTime(data_->qStartTime());
        else if(prevState_.startDt < data_->qEndTime())
            ui_->fromTimeEdit->setDateTime(prevState_.startDt);
    }
    else
    {
        ui_->fromTimeEdit->setDateTime(data_->qStartTime());
    }

    ui_->toTimeEdit->setMinimumDateTime(data_->qStartTime());
    ui_->toTimeEdit->setMaximumDateTime(data_->qEndTime());

    if(prevState_.valid)
    {
        if(prevState_.endDt >= data_->qEndTime() || prevState_.fullEnd)
            ui_->toTimeEdit->setDateTime(data_->qEndTime());
        else if(prevState_.endDt > data_->qStartTime())
            ui_->toTimeEdit->setDateTime(prevState_.endDt);
    }
    else
    {
        ui_->toTimeEdit->setDateTime(data_->qEndTime());
    }

    ignoreTimeEdited_=false;

    view_->setPeriod(ui_->fromTimeEdit->dateTime(),ui_->toTimeEdit->dateTime());

    model_->setData(data_);
}

//Determine missing types
void TimelineWidget::determineNodeTypes()
{
    if(typesDetermined_)
        return;

    if(ServerHandler *sh=ServerHandler::find(serverName_.toStdString()))
    {
        ViewerUtil::setOverrideCursor(QCursor(Qt::WaitCursor));

        for(size_t i=0; i < data_->items().size() ;i++)
        {
            if(data_->items()[i].type() == TimelineItem::UndeterminedType)
            {
                if(VNode *vn=sh->vRoot()->find(data_->items()[i].path()))
                {
                    if(vn->isTask())
                        data_->setItemType(i,TimelineItem::TaskType);
                    else if(vn->isFamily())
                        data_->setItemType(i,TimelineItem::FamilyType);
                }
            }
        }

        ViewerUtil::restoreOverrideCursor();
    }

    typesDetermined_=true;
}

void TimelineWidget::writeSettings(VComboSettings* vs)
{
    int cbIdx=ui_->sortCombo->currentIndex();
    if(cbIdx != -1)
    {
        vs->put("sortMode", ui_->sortCombo->currentData().toString().toStdString());
    }

    vs->put("sortOrder",ui_->sortUpTb->isChecked()?"asc":"desc");
    vs->put("showChanged",ui_->showChangedTb->isChecked());

    view_->writeSettings(vs);
}

void TimelineWidget::readSettings(VComboSettings* vs)
{
    //at this point the model is empty so it is cheap to call sort

    //sort mode
    QString sortMode=QString::fromStdString(vs->get<std::string>("sortMode",std::string()));
    ViewerUtil::initComboBoxByData(sortMode,ui_->sortCombo);

    //sort order
    QString sortOrder=QString::fromStdString(vs->get<std::string>("sortOrder",std::string()));
    if(sortOrder == "asc")
    {
        ui_->sortUpTb->setChecked(true);
    }
    else if(sortOrder == "desc")
    {
        ui_->sortDownTb->setChecked(true);
    }

    ui_->showChangedTb->setChecked(vs->get<bool>("showChanged",true));

    view_->readSettings(vs);
}
