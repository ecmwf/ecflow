//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "TableNodeSortModel.hpp"

#include "TableNodeModel.hpp"
#include "ModelColumn.hpp"

TableNodeSortModel::TableNodeSortModel(TableNodeModel* nodeModel,QObject *parent) :
		QSortFilterProxyModel(parent),
        nodeModel_(nodeModel),
        skipSort_(false)
{
    Q_ASSERT(nodeModel_);
    //connect(nodeModel_,SIGNAL(filterChanged()),
    //		this,SLOT(slotFilterChanged()));

	QSortFilterProxyModel::setSourceModel(nodeModel_);

    setDynamicSortFilter(false);
}

TableNodeSortModel::~TableNodeSortModel()
= default;

VInfo_ptr TableNodeSortModel::nodeInfo(const QModelIndex& index)
{
	return nodeModel_->nodeInfo(mapToSource(index));
}

QModelIndex TableNodeSortModel::infoToIndex(VInfo_ptr info)
{
	return mapFromSource(nodeModel_->infoToIndex(info));
}

QModelIndex TableNodeSortModel::nodeToIndex(const VNode *node)
{
	return mapFromSource(nodeModel_->nodeToIndex(node));
}

void TableNodeSortModel::selectionChanged(QModelIndexList lst)
{
    QModelIndexList lstm;
    Q_FOREACH(QModelIndex idx,lst)
        lstm << mapToSource(idx);

    nodeModel_->selectionChanged(lstm);
}


bool TableNodeSortModel::lessThan(const QModelIndex &left,
                                  const QModelIndex &right) const
{
    if(skipSort_)
        return true;

    auto id=static_cast<TableNodeModel::ColumnType>(left.column());

    if(id == TableNodeModel::PathColumn)
        return left.row() < right.row();

    else if(id == TableNodeModel::MeterColumn)
    {
        return left.data(AbstractNodeModel::SortRole).toInt() <
               right.data(AbstractNodeModel::SortRole).toInt();
    }

    else if(id == TableNodeModel::StatusChangeColumn)
    {
        return left.data(AbstractNodeModel::SortRole).toUInt() <
               right.data(AbstractNodeModel::SortRole).toUInt();
    }

    QVariant leftData = nodeModel_->data(left);
    QVariant rightData = nodeModel_->data(right);

    return leftData.toString() < rightData.toString();
}

void TableNodeSortModel::removeColumn(QString name)
{
    nodeModel_->removeColumn(name);
}

ModelColumn* TableNodeSortModel::columns() const
{
     return nodeModel_->columns();
}


