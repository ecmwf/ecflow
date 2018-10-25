//============================================================================
// Copyright 2009-2018 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef TIMELINEWIDGET_HPP
#define TIMELINEWIDGET_HPP

#include <QWidget>

class TimelineData;
class TimelineModel;
class TimelineView;

namespace Ui {
    class TimelineWidget;
}

//the main widget containing all components
class TimelineWidget : public QWidget
{
Q_OBJECT

public:
    explicit TimelineWidget(QWidget *parent=0);
    ~TimelineWidget();

    void clear();
    void load(QString logFile,int numOfRows=0);
    void load(QString serverName, QString host, QString port, QString logFile,int numOfRows=0);
    QString logFile() const {return logFile_;}

protected Q_SLOTS:
   void slotReload();
   void slotWholePeriod();
   void slotToday();
   void slotYesterday();

private:
    void load();
    void updateInfoLabel();
    void setAllVisible(bool b);

    Ui::TimelineWidget* ui_;
    QString serverName_;
    QString host_;
    QString port_;
    QString logFile_;
    int numOfRows_;

    TimelineData* data_;
    TimelineModel* model_;
    TimelineView* view_;
};

#endif // TIMELINEWIDGET_HPP
