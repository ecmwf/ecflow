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
     return 2;
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
        if(index.column() == 0)
            return QString::fromStdString(data_->items()[row].path());
        else
            return QVariant();
    }
    else if(role == Qt::UserRole)
    {
        if(index.column() == 0)
            return data_->items()[row].isTask();
        else
            return QVariant();
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
        case 0:
            return "Path";
        case 1:
            return "Time";
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

//===========================================
//
// TimelineSortModel
//
//===========================================

TimelineSortModel::TimelineSortModel(TimelineModel* tlModel,QObject *parent) :
        QSortFilterProxyModel(parent),
        tlModel_(tlModel),
        skipSort_(false),
        taskFilter_(false)
{
    Q_ASSERT(tlModel_);
    //connect(nodeModel_,SIGNAL(filterChanged()),
    //		this,SLOT(slotFilterChanged()));

    QSortFilterProxyModel::setSourceModel(tlModel_);

    setDynamicSortFilter(true);
}

TimelineSortModel::~TimelineSortModel()
{
}

#if 0
void TimelineSortModel::selectionChanged(QModelIndexList lst)
{
    QModelIndexList lstm;
    Q_FOREACH(QModelIndex idx,lst)
        lstm << mapToSource(idx);

    nodeModel_->selectionChanged(lstm);
}
#endif

void TimelineSortModel::setPathFilter(QString pathFilter)
{
    pathFilter_=pathFilter;
    invalidate();
}

void TimelineSortModel::setTaskFilter(bool taskFilter)
{
    taskFilter_=taskFilter;
    invalidate();
}

bool TimelineSortModel::lessThan(const QModelIndex &left,
                                 const QModelIndex &right) const
{
    return true;

    if(skipSort_)
        return true;

    if(left.column() == 0)
    {
        QVariant leftData = tlModel_->data(left);
        QVariant rightData = tlModel_->data(right);

        return leftData.toString() < rightData.toString();
    }
    return true;
}

bool TimelineSortModel::filterAcceptsRow(int sourceRow, const QModelIndex &/*sourceParent*/) const
{
    return true;
    bool matched=true;
    if(!pathFilter_.isEmpty())
    {
        matched=tlModel_->data(tlModel_->index(sourceRow,0)).toString().contains(pathFilter_);
    }

    if(matched && taskFilter_)
    {
        matched=tlModel_->data(tlModel_->index(sourceRow,0),Qt::UserRole).toBool();
    }

    return matched;
}
