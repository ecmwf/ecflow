//============================================================================
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "TimelineInfoDailyWidget.hpp"

#include <QtGlobal>
#include <QBuffer>
#include <QCloseEvent>
#include <QDialogButtonBox>
#include <QFileInfo>
#include <QPainter>
#include <QSettings>
#include <QToolButton>
#include <QVBoxLayout>

#include "SessionHandler.hpp"
#include "TextFormat.hpp"
#include "TimelineData.hpp"
#include "TimelineHeaderView.hpp"
#include "TimelineInfoDelegate.hpp"
#include "TimelineModel.hpp"
#include "TimelineView.hpp"
#include "TextFormat.hpp"
#include "UiLog.hpp"
#include "ViewerUtil.hpp"
#include "VNState.hpp"
#include "WidgetNameProvider.hpp"

#include "ui_TimelineInfoDailyWidget.h"


TimelineInfoDailyWidget::TimelineInfoDailyWidget(QWidget *parent) :
    ui_(new Ui::TimelineInfoDailyWidget),
    numOfRows_(0),
    tlEndTime_(0),
    ignoreTimeEdited_(false)
{
    ui_->setupUi(this);

    ui_->zoomInTb->setDefaultAction(ui_->actionZoomIn);
    ui_->zoomOutTb->setDefaultAction(ui_->actionZoomOut);

    ui_->view->setZoomActions(ui_->actionZoomIn,ui_->actionZoomOut);

    ui_->fromTimeEdit->setTime(QTime(0,0,0));
    ui_->toTimeEdit->setTime(QTime(23,59,59));

    connect(ui_->fromTimeEdit,SIGNAL(timeChanged(QTime)),
            this,SLOT(slotStartChanged(QTime)));

    connect(ui_->toTimeEdit,SIGNAL(timeChanged(QTime)),
            this,SLOT(slotEndChanged(QTime)));

    connect(ui_->startTb,SIGNAL(clicked()),
            this,SLOT(slotResetStart()));

    connect(ui_->endTb,SIGNAL(clicked()),
            this,SLOT(slotResetEnd()));

    connect(ui_->wholePeriodTb,SIGNAL(clicked()),
            this,SLOT(slotWholePeriod()));

    connect(ui_->view,SIGNAL(periodSelected(QTime,QTime)),
            this,SLOT(slotPeriodSelectedInView(QTime,QTime)));

    connect(ui_->view,SIGNAL(periodBeingZoomed(QTime,QTime)),
            this,SLOT(slotPeriodBeingZoomedInView(QTime,QTime)));
}

void TimelineInfoDailyWidget::slotPeriodSelectedInView(QTime start,QTime end)
{
    //Q_ASSERT(data_);
    //if(start >= data_->qStartTime() && end <= data_->qEndTime())
    {
        ignoreTimeEdited_=true;
        ui_->fromTimeEdit->setTime(start);
        ui_->toTimeEdit->setTime(end);
        ignoreTimeEdited_=false;
    }
    checkButtonState();
}

void TimelineInfoDailyWidget::slotPeriodBeingZoomedInView(QTime start,QTime end)
{
    //Q_ASSERT(data_);
    //if(start >= data_->qStartTime() && end <= data_->qEndTime())
    {
        ignoreTimeEdited_=true;
        ui_->fromTimeEdit->setTime(start);
        ui_->toTimeEdit->setTime(end);
        ignoreTimeEdited_=false;
    }
    //checkButtonState();
}

void TimelineInfoDailyWidget::slotStartChanged(const QTime& dt)
{
    if(!ignoreTimeEdited_)
    {
        ui_->view->setStartTime(dt);
    }
    checkButtonState();
}

void TimelineInfoDailyWidget::slotEndChanged(const QTime& dt)
{
    if(!ignoreTimeEdited_)
    {
        ui_->view->setEndTime(dt);
    }
    checkButtonState();
}

void TimelineInfoDailyWidget::slotResetStart()
{
    ui_->fromTimeEdit->setTime(QTime(0,0,0));
    checkButtonState();
}

void TimelineInfoDailyWidget::slotResetEnd()
{
    ui_->toTimeEdit->setTime(QTime(23,59,59));
    checkButtonState();
}

void TimelineInfoDailyWidget::slotWholePeriod()
{
    //Q_ASSERT(data_);
    ignoreTimeEdited_=true;
    ui_->fromTimeEdit->setTime(QTime(0,0,0));
    ui_->toTimeEdit->setTime(QTime(23,59,59));
    ignoreTimeEdited_=false;
    ui_->view->setPeriod(QTime(0,0,0), QTime(23,59,59));
    checkButtonState();
}

void TimelineInfoDailyWidget::checkButtonState()
{
    bool fromStart=(ui_->fromTimeEdit->time() == QTime(0,0,0));
    bool toEnd=(ui_->toTimeEdit->time() == QTime(23,59,59));

    ui_->startTb->setEnabled(!fromStart);
    ui_->endTb->setEnabled(!toEnd);
    ui_->wholePeriodTb->setEnabled(!fromStart || !toEnd);
}

void TimelineInfoDailyWidget::load(TimelineItem* data,unsigned int viewStartDateSec,unsigned int viewEndDateSec,
             unsigned int endDateSec)
//void TimelineInfoDailyWidget::load(QString host, QString port,TimelineData *tlData, int itemIndex,QDateTime viewStartDate,
//                              QDateTime viewEndDate)
{
 #if 0
    Q_ASSERT(tlData);
    host_=host;
    port_=port;
    data_=tlData->items()[itemIndex];
    tlEndTime_=tlData->endTime();

    QColor col(39,49,101);
    QColor colText(30,30,30);
    QString title=Viewer::formatBoldText("Node: ",col) + QString::fromStdString(data_.path());

#endif

    ui_->view->load(data,viewStartDateSec,viewEndDateSec,endDateSec);
    ui_->view->setPeriod(ui_->fromTimeEdit->time(),ui_->toTimeEdit->time());
    checkButtonState();
}

void TimelineInfoDailyWidget::readSettings(QSettings& vs)
{
    vs.beginGroup("dailyWidget");
    ui_->view->readSettings(vs);
    vs.endGroup();
}

void TimelineInfoDailyWidget::writeSettings(QSettings& vs)
{
    vs.beginGroup("dailyWidget");
    ui_->view->writeSettings(vs);
    vs.endGroup();
}
