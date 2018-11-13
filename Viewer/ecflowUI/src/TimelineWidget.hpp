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
#include <QDateTime>

#include "VFile.hpp"

class TimelineData;
class TimelineModel;
class TimelineSortModel;
class TimelineView;
class VComboSettings;
class VFileTransfer;

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
    void load(QString logFile);
    void load(QString serverName, QString host, QString port, QString logFile,
              const std::vector<std::string>& suites);
    QString logFile() const {return logFile_;}

    void writeSettings(VComboSettings* vs);
    void readSettings(VComboSettings* vs);

protected Q_SLOTS:
   void slotReload();
   void slotResetStart();
   void slotResetEnd();
   void slotWholePeriod();
   void slotStartChanged(const QDateTime&);
   void slotEndChanged(const QDateTime&);
   void slotPathFilter(QString);
   void slotTaskOnly(bool);
   void slotPeriodSelectedInView(QDateTime,QDateTime);
   void slotPeriodBeingZoomedInView(QDateTime,QDateTime);
   void slotLookup(QString);
   void slotCopyPath(QString);
   void slotFileTransferFinished();
   void slotFileTransferFailed(QString);
   void slotFileTransferStdOutput(QString msg);
   void slotLogLoadProgress(size_t current, size_t total);

private:
    void load();
    void loadCore(QString logFile);
    void updateInfoLabel(bool showDetails=true);
    void setAllVisible(bool b);
    void checkButtonState();
    void determineNodeTypes();

    Ui::TimelineWidget* ui_;
    QString serverName_;
    QString host_;
    QString port_;
    QString logFile_;
    VFile_ptr tmpLogFile_;
    size_t maxReadSize_;
    std::vector<std::string> suites_;

    TimelineData* data_;
    TimelineModel* model_;
    TimelineSortModel* sortModel_;
    TimelineView* view_;
    bool ignoreTimeEdited_;
    bool beingCleared_;

    bool typesDetermined_;
    bool localLog_;
    bool logLoaded_;
    bool logTransferred_;
    VFileTransfer* fileTransfer_;
    QDateTime transferredAt_;
};

#endif // TIMELINEWIDGET_HPP
