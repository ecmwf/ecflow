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

#include "MainWindow.hpp"
#include "ServerHandler.hpp"
#include "TimelineData.hpp"
#include "TimelineModel.hpp"
#include "TimelineView.hpp"
#include "TextFormat.hpp"
#include "UiLog.hpp"
#include "ViewerUtil.hpp"
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
    maxReadSize_(25*1024*1024),
    loadFailed_(false),
    data_(0),
    ignoreTimeEdited_(false)
{
    ui_->setupUi(this);

    //message label
    ui_->messageLabel->hide();

    data_=new TimelineData;

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

#if QT_VERSION >= QT_VERSION_CHECK(4, 7, 0)
    ui_->pathFilterLe->setPlaceholderText(tr("Filter"));
#endif

#if QT_VERSION >= QT_VERSION_CHECK(5, 2, 0)
    ui_->pathFilterLe->setClearButtonEnabled(true);
#endif

    ui_->fromTimeEdit->setDisplayFormat("hh:mm:ss dd-MMM-2018");
    ui_->toTimeEdit->setDisplayFormat("hh:mm:ss dd-MMM-2018");

    connect(ui_->pathFilterLe,SIGNAL(textChanged(QString)),
            this,SLOT(slotPathFilter(QString)));

    connect(ui_->taskOnlyTb,SIGNAL(clicked(bool)),
            this,SLOT(slotTaskOnly(bool)));

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
}

TimelineWidget::~TimelineWidget()
{
    if(data_)
        delete data_;
}

void TimelineWidget::clear()
{
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
    loadFailed_=false;
    //ui_->startTe->clear();
    //ui_->endTe->clear();

    setAllVisible(false);
}

void TimelineWidget::updateInfoLabel()
{
    QColor col(39,49,101);
    QString txt=Viewer::formatBoldText("Log file: ",col) + logFile_;
    txt+=Viewer::formatBoldText(" Server: ",col) + serverName_ +
         Viewer::formatBoldText(" Host: ",col) + host_ +
         Viewer::formatBoldText(" Port: ",col) + port_;

    if(!loadFailed_ && data_->loadTried() && !data_->isFullRead())
    {
        txt+=Viewer::formatBoldText(" Log entries: ",col) +
           "parsed last " + QString::number(maxReadSize_/(1024*1024)) + " MB of file (maximum reached)";
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

void TimelineWidget::slotTaskOnly(bool taskFilter)
{
    sortModel_->setTaskFilter(taskFilter);
}

void TimelineWidget::slotPathFilter(QString pattern)
{
    sortModel_->setPathFilter(pattern);
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
    clear();

    serverName_=serverName;
    host_=host;
    port_=port;
    logFile_=logFile;
    suites_=suites;

    updateInfoLabel();

    //setAllVisible(false);
    loadFailed_=false;
    ViewerUtil::setOverrideCursor(QCursor(Qt::WaitCursor));

    try
    {
        data_->loadLogFile(logFile_.toStdString(),maxReadSize_,suites);
    }
    catch(std::runtime_error e)
    {
        loadFailed_=true;
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

    setAllVisible(true);
    updateInfoLabel();

    //Determine missing types
    if(ServerHandler *sh=ServerHandler::find(serverName_.toStdString()))
    {
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
    }

    ViewerUtil::restoreOverrideCursor();

    ui_->fromTimeEdit->setMinimumDateTime(data_->qStartTime());
    ui_->fromTimeEdit->setMaximumDateTime(data_->qEndTime());
    ui_->fromTimeEdit->setDateTime(data_->qStartTime());

    ui_->toTimeEdit->setMinimumDateTime(data_->qStartTime());
    ui_->toTimeEdit->setMaximumDateTime(data_->qEndTime());
    ui_->toTimeEdit->setDateTime(data_->qEndTime());

    view_->setPeriod(data_->qStartTime(),data_->qEndTime());

    model_->setData(data_);
}


void TimelineWidget::writeSettings(VComboSettings* vs)
{
    view_->writeSettings(vs);
}

void TimelineWidget::readSettings(VComboSettings* vs)
{
    view_->readSettings(vs);
}
