//============================================================================
// Copyright 2009-2018 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef TIMELINEMODEL_CPP
#define TIMELINEMODEL_CPP

#include <QAbstractItemModel>
#include <QSortFilterProxyModel>
#include <QDateTime>
#include <QSet>

class TimelineData;

class TimelineModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    enum CustomItemRole {PathSortRole = Qt::UserRole+1, TimeSortRole = Qt::UserRole+2,
                        UnchangedRole = Qt::UserRole+3, MeanDurationRole = Qt::UserRole+4,
                        DurationStatRole = Qt::UserRole+5,QtSortRole = Qt::UserRole+6};

    enum ColumnType {PathColumn=0, TimelineColumn=1, SubmittedDurationColumn=2,
                     ActiveDurationColumn=3};

    explicit TimelineModel(QObject *parent=0);
    ~TimelineModel();

    int columnCount (const QModelIndex& parent = QModelIndex() ) const;
    int rowCount (const QModelIndex& parent = QModelIndex() ) const;

    Qt::ItemFlags flags ( const QModelIndex & index) const;
    QVariant data (const QModelIndex& , int role = Qt::DisplayRole ) const;
    QVariant headerData(int,Qt::Orientation,int role = Qt::DisplayRole ) const;

    QModelIndex index (int, int, const QModelIndex& parent = QModelIndex() ) const;
    QModelIndex parent (const QModelIndex & ) const;

    void setData(TimelineData* data);

    TimelineData* data() const {return data_;}
    void clearData();
    bool hasData() const;
    void setPeriod(QDateTime t1,QDateTime t2);
    void setStartDate(QDateTime t);
    void setEndDate(QDateTime t);

Q_SIGNALS:
    void periodChanged();

protected:
    TimelineData* data_;
    QDateTime startDate_;
    QDateTime endDate_;
};

class TimelineSortModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    TimelineSortModel(TimelineModel*,QObject *parent=0);
    ~TimelineSortModel();

    enum SortMode {PathSortMode, TimeSortMode, QtSortMode};

    //From QSortFilterProxyModel:
    //we set the source model in the constructor. So this function should not do anything.
    void setSourceModel(QAbstractItemModel*) {}
    TimelineModel* tlModel() const {return tlModel_;}

    void selectionChanged(QModelIndexList lst);
    void setSortMode(SortMode);
    void setSortDirection(bool ascending);
    void setSkipSort(bool b) {skipSort_=b;}
    void setPathFilter(QString);
    void setTaskFilter(bool);
    void setShowChangedOnly(bool);

protected Q_SLOTS:
    void slotPeriodChanged();

Q_SIGNALS:
    void invalidateCalled();

protected:
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const;
    bool filterAcceptsRow(int sourceRow, const QModelIndex &/*sourceParent*/) const;

    TimelineModel* tlModel_;
    bool skipSort_;
    SortMode sortMode_;
    bool ascending_;
    QString pathFilter_;
    bool taskFilter_;
    QRegExp pathFilterRx_;
    bool showChangedOnly_;
};

#endif // TIMELINEMODEL_CPP

