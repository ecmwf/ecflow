/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_TimelineInfoDailyWidget_HPP
#define ecflow_viewer_TimelineInfoDailyWidget_HPP

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

// the main widget containing all components
class TimelineInfoDailyWidget : public QWidget {
    Q_OBJECT

public:
    explicit TimelineInfoDailyWidget(QWidget* parent = nullptr);
    ~TimelineInfoDailyWidget() override = default;

    void clear() {}
    // void load(QString host,QString port,TimelineData*,int,QDateTime,QDateTime);
    void load(TimelineItem*, unsigned int viewStartDateSec, unsigned int viewEndDateSec, unsigned int endDateSec);

    void readSettings(QSettings&);
    void writeSettings(QSettings&);

protected Q_SLOTS:
    void slotPeriodSelectedInView(QTime start, QTime end);
    void slotPeriodBeingZoomedInView(QTime start, QTime end);
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

    // TimelineInfoDailyModel* dailyModel_;
    // NodeTimelineHeader* dailyHeader_;
};

#endif /* ecflow_viewer_TimelineInfoDailyWidget_HPP */
