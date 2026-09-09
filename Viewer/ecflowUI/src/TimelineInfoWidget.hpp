/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_TimelineInfoWidget_HPP
#define ecflow_viewer_TimelineInfoWidget_HPP

#include <QAbstractItemModel>
#include <QDialog>
#include <QSettings>
#include <QWidget>

#include "TimelineData.hpp"

class QDateTime;
class TimelineData;
class TimelineItem;
class VNState;
class NodeTimelineHeader;

namespace Ui {
class TimelineInfoWidget;
}

class TimelineInfoModel : public QAbstractItemModel {
public:
    explicit TimelineInfoModel(QObject* parent = nullptr);
    ~TimelineInfoModel() override;

    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;

    Qt::ItemFlags flags(const QModelIndex& index) const override;
    QVariant data(const QModelIndex&, int role = Qt::DisplayRole) const override;
    QVariant headerData(int, Qt::Orientation, int role = Qt::DisplayRole) const override;

    QModelIndex index(int, int, const QModelIndex& parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex&) const override;

    void resetData(TimelineItem*, unsigned int viewStartDateSec, unsigned int viewEndDateSec, unsigned int endDateSec);
    void clearData();
    bool hasData() const;
    void determineRowsInPeriod();

protected:
    TimelineItem* data_{nullptr};
    unsigned int viewStartDateSec_{0};
    unsigned int viewEndDateSec_{0};
    unsigned int endDateSec_{0};
    int firstRowInPeriod_{-1};
    int lastRowInPeriod_{-1};
};

// the main widget containing all components
class TimelineInfoWidget : public QWidget {
    Q_OBJECT

    friend class TimelineInfoDialog;

public:
    explicit TimelineInfoWidget(QWidget* parent = nullptr);
    ~TimelineInfoWidget() override {}

    void clear();
    void load(QString host, QString port, TimelineData*, int, QDateTime, QDateTime);
    int itemIndex() const { return itemIndex_; }

private:
    void createSummary();
    void createSummary(QString& txt, VNState* state);
    QPixmap makeBoxPlot(VNState* state, int num, int mean, TimelineItemStats stats);
    void updateInfoLabel() {}
    void readSettings(QSettings& settings);
    void writeSettings(QSettings& settings);

    Ui::TimelineInfoWidget* ui_;
    QString serverName_;
    QString host_;
    QString port_;
    QString logFile_;
    int numOfRows_;

    TimelineItem data_;
    int currentRow_;
    TimelineInfoModel* model_;
    int tlEndTime_;
    int itemIndex_{-1};

    // TimelineInfoDailyModel* dailyModel_;
    NodeTimelineHeader* dailyHeader_;

    static bool columnsAdjusted_;
};

class TimelineInfoDialog : public QDialog {
public:
    explicit TimelineInfoDialog(QWidget* parent = nullptr);
    ~TimelineInfoDialog() override;

    TimelineInfoWidget* infoW_;

protected:
    void closeEvent(QCloseEvent* event) override;
    void readSettings();
    void writeSettings();
};

#endif /* ecflow_viewer_TimelineInfoWidget_HPP */
