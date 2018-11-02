//============================================================================
// Copyright 2009-2018 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef TIMELINEINFOWIDGET_HPP
#define TIMELINEINFOWIDGET_HPP

#include <QDialog>
#include <QWidget>
#include <QAbstractItemModel>

#include "TimelineData.hpp"

class QDateTime;
class TimelineData;
class TimelineItem;

namespace Ui {
    class TimelineInfoWidget;
}

class TimelineInfoModel : public QAbstractItemModel
{
public:
    explicit TimelineInfoModel(QObject *parent=0);
    ~TimelineInfoModel();

    int columnCount (const QModelIndex& parent = QModelIndex() ) const;
    int rowCount (const QModelIndex& parent = QModelIndex() ) const;

    Qt::ItemFlags flags ( const QModelIndex & index) const;
    QVariant data (const QModelIndex& , int role = Qt::DisplayRole ) const;
    QVariant headerData(int,Qt::Orientation,int role = Qt::DisplayRole ) const;

    QModelIndex index (int, int, const QModelIndex& parent = QModelIndex() ) const;
    QModelIndex parent (const QModelIndex & ) const;

    void setData(TimelineItem*);
    void clearData();
    bool hasData() const;

protected:
    TimelineItem* data_;
};


class TimelineInfoWidget;

class TimelineInfoDialog : public QDialog
{
public:
    TimelineInfoDialog(QWidget* parent=0);
    TimelineInfoWidget* infoW_;
};

//the main widget containing all components
class TimelineInfoWidget : public QWidget
{
Q_OBJECT

public:
    explicit TimelineInfoWidget(QWidget *parent=0);
    ~TimelineInfoWidget() {}

    void clear() {}
    void load(QString host,QString port,const TimelineItem& data);

private:
    void updateInfoLabel() {}

    Ui::TimelineInfoWidget* ui_;
    QString serverName_;
    QString host_;
    QString port_;
    QString logFile_;
    int numOfRows_;

    TimelineItem data_;
    int currentRow_;
    TimelineInfoModel* model_;
};

#endif // TIMELINEINFOWIDGET_HPP
