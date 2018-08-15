//============================================================================
// Copyright 2009-2018 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "TimelineWidget.hpp"

#include <QFileInfo>

#include "TimelineData.hpp"
#include "TimelineModel.hpp"
#include "TimelineView.hpp"
#include "TextFormat.hpp"

#include "ui_TimelineWidget.h"

//=======================================================
//
// LogLoadWidget
//
//=======================================================

TimelineWidget::TimelineWidget(QWidget *parent) :
    ui_(new Ui::TimelineWidget),
    numOfRows_(0),
    data_(0)
{
    ui_->setupUi(this);

    //message label
    ui_->messageLabel->hide();

    data_=new TimelineData;

    //Log contents
    model_=new TimelineModel(this);

    model_->setData(data_);

    view_=new TimelineView(model_,this);
    view_->setProperty("log","1");
    view_->setProperty("log","1");
    view_->setRootIsDecorated(false);
    view_->setModel(model_);
    view_->setUniformRowHeights(true);
    view_->setAlternatingRowColors(false);
    //ui_->view->setItemDelegate(new LogDelegate(this));
    view_->setContextMenuPolicy(Qt::ActionsContextMenu);

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

    connect(ui_->reloadTb,SIGNAL(clicked()),
            this,SLOT(slotReload()));

    //ui_->timeWidget->setStyleSheet("#timeWidget{background-color: rgb(212,212,212);}");
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
    logFile_.clear();
    serverName_.clear();
    host_.clear();
    port_.clear();
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

#if 0
    QDateTime startDt=viewHandler_->data()->startTime();
    QDateTime endDt=viewHandler_->data()->endTime();
    txt+=Viewer::formatBoldText(" Full period: ",col) +
            startDt.toString("yyyy-MM-dd hh:mm:ss") + Viewer::formatBoldText(" to ",col) +
            endDt.toString("yyyy-MM-dd hh:mm:ss");

    int maxNum=viewHandler_->data()->maxNumOfRows();
    int num=viewHandler_->data()->numOfRows();
    if(maxNum != 0 && num == abs(maxNum))
    {
        txt+=Viewer::formatBoldText(" Log entries: ",col) +
           "last " + QString::number(abs(maxNum)) + " rows read (maximum reached)";
    }
#endif

    ui_->logInfoLabel->setText(txt);

    //TODO: we need a better implementation
   // ui_->timeWidget->hide();
}

void TimelineWidget::setAllVisible(bool b)
{
    //ui_->viewTab->setVisible(b);
    //ui_->logView->setVisible(b);
    //ui_->timeWidget->setVisible(b);
}

void TimelineWidget::slotReload()
{
    if(!serverName_.isEmpty() && numOfRows_ != 0)
    {
        load(serverName_, host_, port_, logFile_,numOfRows_);
    }
}

void TimelineWidget::load(QString logFile,int numOfRows)
{
    load("","","",logFile,numOfRows);
}

void TimelineWidget::load(QString serverName, QString host, QString port, QString logFile,int numOfRows)
{
    clear();

    serverName_=serverName;
    host_=host;
    port_=port;
    logFile_=logFile;
    numOfRows_=numOfRows;

    updateInfoLabel();

    QFileInfo fInfo(logFile);
    if(!fInfo.exists())
    {
        ui_->messageLabel->showError("The specified log file does not exist!");
        return;
    }

    if(!fInfo.isReadable())
    {
        ui_->messageLabel->showError("The specified log file is not readable!");
        return;
    }

    if(!fInfo.isFile())
    {
        ui_->messageLabel->showError("The specified log file is not a file!");
        return;
    }

    setAllVisible(true);

    try
    {
        data_->loadLogFile(logFile_.toStdString(),numOfRows);
        //viewHandler_->load(logFile_.toStdString(),numOfRows);
    }
    catch(std::runtime_error e)
    {
        ui_->messageLabel->showError(e.what());
        setAllVisible(false);
    }

    model_->setData(data_);

    //logModel_->loadFromFile(logFile_.toStdString(),viewHandler_->data()->startPos());

    /*QDateTime startTime=viewHandler_->data()->startTime();
    QDateTime endTime=viewHandler_->data()->endTime();
    ui_->startTe->setMinimumDateTime(startTime);
    ui_->startTe->setMaximumDateTime(endTime);
    ui_->startTe->setDateTime(startTime);
    ui_->endTe->setMinimumDateTime(startTime);
    ui_->endTe->setMaximumDateTime(endTime);
    ui_->endTe->setDateTime(endTime);*/

    updateInfoLabel();
}


