//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "NodeFilterModel.hpp"

#include "AbstractNodeModel.hpp"

NodeFilterModel::NodeFilterModel(QObject *parent) :
		QSortFilterProxyModel(parent)
{
}

NodeFilterModel::~NodeFilterModel()
{
}

void NodeFilterModel::setSourceModel(QAbstractItemModel* m)
{
	connect(m,SIGNAL(filterChanged()),
		this,SLOT(slotFilterChanged()));

	QSortFilterProxyModel::setSourceModel(m);
}


bool NodeFilterModel::filterAcceptsRow(int sourceRow,const QModelIndex& sourceParent) const
{
	QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);
	return sourceModel()->data(index,AbstractNodeModel::FilterRole).toBool();
}

void NodeFilterModel::slotFilterChanged()
{
	invalidateFilter();
}


