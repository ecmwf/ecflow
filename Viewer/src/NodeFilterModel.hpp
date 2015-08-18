//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef NODEFILTERMODEL_H
#define NODEFILTERMODEL_H

#include <QSortFilterProxyModel>

#include "VInfo.hpp"

class AbstractNodeModel;
class NodeFilterDef;

class NodeFilterModel : public QSortFilterProxyModel
{
Q_OBJECT

public:
	NodeFilterModel(AbstractNodeModel*,QObject *parent=0);
	~NodeFilterModel();

	bool filterAcceptsRow(int,const QModelIndex &) const;

	//From QSortFilterProxyModel:
	//we set the source model in the constructor. So this function should not do anything.
	void setSourceModel(QAbstractItemModel*) {};

	VInfo_ptr nodeInfo(const QModelIndex&);
	QModelIndex infoToIndex(VInfo_ptr);
	QModelIndex nodeToIndex(const VNode *node);

public Q_SLOTS:
	void slotFilterChanged();

protected:
	AbstractNodeModel* nodeModel_;
};

#endif
