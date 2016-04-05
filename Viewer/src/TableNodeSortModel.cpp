//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "TableNodeSortModel.hpp"

#include "AbstractNodeModel.hpp"

TableNodeSortModel::TableNodeSortModel(AbstractNodeModel* nodeModel,QObject *parent) :
		QSortFilterProxyModel(parent),
		nodeModel_(nodeModel)
{
    //connect(nodeModel_,SIGNAL(filterChanged()),
    //		this,SLOT(slotFilterChanged()));

	QSortFilterProxyModel::setSourceModel(nodeModel_);

    //setDynamicSortFilter(true);
}

TableNodeSortModel::~TableNodeSortModel()
{
}

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
