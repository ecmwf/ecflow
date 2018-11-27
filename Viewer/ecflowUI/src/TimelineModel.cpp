//============================================================================
// Copyright 2009-2018 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "TimelineModel.hpp"

#include "TimelineData.hpp"

#include <QDebug>

TimelineModel::TimelineModel(QObject *parent) :
          QAbstractItemModel(parent), data_(0)
{
}

TimelineModel::~TimelineModel()
{
}

void TimelineModel::setData(TimelineData *data)
{
    beginResetModel();
    data_=data;
    endResetModel();
}

void TimelineModel::clearData()
{
    beginResetModel();
    data_=0;
    endResetModel();
}

bool TimelineModel::hasData() const
{
    return data_;
}

int TimelineModel::columnCount( const QModelIndex& /*parent */) const
{
     return 4;
}

int TimelineModel::rowCount( const QModelIndex& parent) const
{
    if(!hasData())
        return 0;

    //Parent is the root:
    if(!parent.isValid())
    {
        return static_cast<int>(data_->size());
    }

    return 0;
}

Qt::ItemFlags TimelineModel::flags ( const QModelIndex & index) const
{
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QVariant TimelineModel::data( const QModelIndex& index, int role ) const
{
    if(!index.isValid() || !hasData())
    {
        return QVariant();
    }

    int row=index.row();
    if(row < 0 || row >= static_cast<int>(data_->size()))
        return QVariant();

    if(role == Qt::DisplayRole)
    {
        if(index.column() == PathColumn)
            return QString::fromStdString(data_->items()[row].path());
        else if(index.column() == TimelineColumn)
            return row;
        else if(index.column() == SubmittedDurationColumn)
            return data_->items()[row].firstSubmittedDuration(startDate_,endDate_);
        else if(index.column() == ActiveDurationColumn)
            return data_->items()[row].firstActiveDuration(startDate_,endDate_);
        else
            return row;
    }

    //sort roles
    else if(role  == PathSortRole)
    {
        if(index.column() ==  PathColumn)
            return static_cast<qint64>(data_->items()[row].sortIndex());
        else
            return QVariant();
    }

    //sort roles
    else if(role  == TimeSortRole)
    {
        unsigned int start=TimelineItem::fromQDateTime(startDate_);
        unsigned int end=TimelineItem::fromQDateTime(endDate_);
        for(size_t i=0; i <= data_->items()[row].size(); i++)
        {
            unsigned int val=data_->items()[row].start_[i];
            if(val >= start)
            {
                if(val <=end)
                    return val;
                else
                    return end+2;
            }
        }
        return end+1;
    }

    //task filter
    else if(role == Qt::UserRole)
    {
        if(index.column() ==  PathColumn)
            return data_->items()[row].isTask();
        else
            return QVariant();
    }

    //filter = unchanged in period
    else if(role  == UnchangedRole)
    {
        unsigned int start=TimelineItem::fromQDateTime(startDate_);
        unsigned int end=TimelineItem::fromQDateTime(endDate_);
        for(size_t i=0; i <= data_->items()[row].size(); i++)
        {
            unsigned int val=data_->items()[row].start_[i];
            if(val >= start && val <= end)
            {
                return false;
            }
        }
        return true;
    }

    //duration of first submitted task in period preceding the first active state
    else if(role  == MeanDurationRole)
    {
        int num=0;
        float meanVal=-1.;
        QVariantList  vals;
        if(index.column() == SubmittedDurationColumn)
        {
            data_->items()[row].meanSubmittedDuration(meanVal,num);
            vals << meanVal << num;
        }
        else if(index.column() == ActiveDurationColumn)
        {
            data_->items()[row].meanActiveDuration(meanVal,num);
            vals << meanVal << num;
        }
        return vals;
    }

    //duration of first submitted task in period preceding the first active state
    else if(role  == DurationStatRole)
    {
    }

    return QVariant();
}

QVariant TimelineModel::headerData( const int section, const Qt::Orientation orient , const int role ) const
{
    if ( orient != Qt::Horizontal || (role != Qt::DisplayRole && role != Qt::UserRole ))
              return QAbstractItemModel::headerData( section, orient, role );

    if(role == Qt::DisplayRole)
    {
        switch(section)
        {
        case PathColumn:
            return tr("Path");
        case TimelineColumn:
            return "";
        case SubmittedDurationColumn:
            return tr("First submitted duration in period");
        case ActiveDurationColumn:
            return tr("First active duration in period");
        default:
            return QVariant();
        }
    }

    return QVariant();
}

QModelIndex TimelineModel::index( int row, int column, const QModelIndex & parent ) const
{
    if(!hasData() || row < 0 || column < 0)
    {
        return QModelIndex();
    }

    //When parent is the root this index refers to a node or server
    if(!parent.isValid())
    {
        return createIndex(row,column);
    }

    return QModelIndex();

}

QModelIndex TimelineModel::parent(const QModelIndex &child) const
{
    return QModelIndex();
}

void TimelineModel::setPeriod(QDateTime t1,QDateTime t2)
{
    startDate_=t1;
    endDate_=t2;
    Q_EMIT periodChanged();
}

void TimelineModel::setStartDate(QDateTime t)
{
    startDate_=t;
    Q_EMIT periodChanged();
}

void TimelineModel::setEndDate(QDateTime t)
{
    endDate_=t;
    Q_EMIT periodChanged();
}

//===========================================
//
// TimelineSortModel
//
//===========================================

TimelineSortModel::TimelineSortModel(TimelineModel* tlModel,QObject *parent) :
        QSortFilterProxyModel(parent),
        tlModel_(tlModel),
        skipSort_(false),
        sortMode_(PathSortMode),
        ascending_(true),
        taskFilter_(false),
        showChangedOnly_(true)
{
    Q_ASSERT(tlModel_);

    connect(tlModel_,SIGNAL(periodChanged()),
            this,SLOT(slotPeriodChanged()));

    QSortFilterProxyModel::setSourceModel(tlModel_);

    setDynamicSortFilter(true);
}

TimelineSortModel::~TimelineSortModel()
{
}

void TimelineSortModel::slotPeriodChanged()
{
    if(sortMode_ == TimeSortMode || showChangedOnly_)
    {
        invalidate();
        Q_EMIT invalidateCalled();
    }
}

void TimelineSortModel::setSortMode(SortMode mode)
{
    if(sortMode_ != mode)
    {
        sortMode_ = mode;
        invalidate();
        Q_EMIT invalidateCalled();
    }
}

void TimelineSortModel::setSortDirection(bool ascending)
{
    ascending_=ascending;
    sort(0,(ascending_)?Qt::AscendingOrder:Qt::DescendingOrder);
}

void TimelineSortModel::setPathFilter(QString pathFilter)
{
    pathFilter_=pathFilter;
    pathFilterRx_=QRegExp(pathFilter_);
    pathFilterRx_.setPatternSyntax(QRegExp::Wildcard);
    pathFilterRx_.setCaseSensitivity(Qt::CaseInsensitive);
    invalidate();
    Q_EMIT invalidateCalled();
}

void TimelineSortModel::setTaskFilter(bool taskFilter)
{
    taskFilter_=taskFilter;    
    invalidate();
    Q_EMIT invalidateCalled();
}

void TimelineSortModel::setShowChangedOnly(bool h)
{
    if(showChangedOnly_ != h)
    {
        showChangedOnly_ = h;
        invalidate();
        Q_EMIT invalidateCalled();
    }
}

bool TimelineSortModel::lessThan(const QModelIndex &left,
                                 const QModelIndex &right) const
{
    if(skipSort_)
        return true;

    if(sortMode_ == PathSortMode)
    {
        return tlModel_->data(left,TimelineModel::PathSortRole).toInt() <
               tlModel_->data(right,TimelineModel::PathSortRole).toInt();
    }
    else if(sortMode_ == TimeSortMode)
    {
         return tlModel_->data(left,TimelineModel::TimeSortRole).toUInt() <
               tlModel_->data(right,TimelineModel::TimeSortRole).toUInt();
    }

    return true;
}

bool TimelineSortModel::filterAcceptsRow(int sourceRow, const QModelIndex &/*sourceParent*/) const
{
    bool matched=true;
    if(!pathFilter_.isEmpty())
    {
        matched=tlModel_->data(tlModel_->index(sourceRow,0)).toString().contains(pathFilterRx_);
    }

    if(matched && taskFilter_)
    {
        matched=tlModel_->data(tlModel_->index(sourceRow,0),Qt::UserRole).toBool();
    }

    if(matched && showChangedOnly_)
    {
        matched=(tlModel_->data(tlModel_->index(sourceRow,0),TimelineModel::UnchangedRole).toBool() == false);
    }

    return matched;
}
