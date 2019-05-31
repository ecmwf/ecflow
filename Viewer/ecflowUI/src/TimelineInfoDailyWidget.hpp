//============================================================================
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef TIMELINEINFODAILYWIDGET_HPP
#define TIMELINEINFODAILYWIDGET_HPP

#include <QSettings>
#include <QWidget>

#include "TimelineData.hpp"

class QDateTime;
class TimelineData;
class TimelineItem;
class VNState;
class NodeTimelineHeader;
class TimelineInfoDailyDelegate;

namespace Ui {
    class TimelineInfoDailyWidget;
}

//the main widget containing all components
class TimelineInfoDailyWidget : public QWidget
{
Q_OBJECT

public:
    explicit TimelineInfoDailyWidget(QWidget *parent=0);
    ~TimelineInfoDailyWidget() {}

    void clear() {}
    //void load(QString host,QString port,TimelineData*,int,QDateTime,QDateTime);
    void load(TimelineItem*,unsigned int viewStartDateSec,unsigned int viewEndDateSec,
                 unsigned int endDateSec);

    void readSettings(QSettings&);
    void writeSettings(QSettings&);

protected Q_SLOTS:
    void slotPeriodSelectedInView(QTime start,QTime end);
    void slotPeriodBeingZoomedInView(QTime start,QTime end);
    void slotStartChanged(const QTime&);
    void slotEndChanged(const QTime&);
    void slotResetStart();
    void slotResetEnd();
    void slotWholePeriod();

private:
    void checkButtonState();

    Ui::TimelineInfoDailyWidget* ui_;
    QString serverName_;
    QString host_;
    QString port_;
    QString logFile_;
    int numOfRows_;

    TimelineItem data_;
    int currentRow_;
    int tlEndTime_;
    bool ignoreTimeEdited_;

    //TimelineInfoDailyModel* dailyModel_;
    //NodeTimelineHeader* dailyHeader_;
};


#endif // TIMELINEINFODAILYWIDGET_HPP
