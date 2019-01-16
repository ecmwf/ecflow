//============================================================================
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef TABLENODEFILTERMODEL_H
#define TABLENODEFILTERMODEL_H

#include <QSortFilterProxyModel>

#include "VInfo.hpp"

class TableNodeModel;
class NodeFilterDef;
class ModelColumn;

class TableNodeSortModel : public QSortFilterProxyModel
{
public:
    TableNodeSortModel(TableNodeModel*,QObject *parent=0);
    ~TableNodeSortModel();

	//From QSortFilterProxyModel:
	//we set the source model in the constructor. So this function should not do anything.
    void setSourceModel(QAbstractItemModel*) {}

	VInfo_ptr nodeInfo(const QModelIndex&);
	QModelIndex infoToIndex(VInfo_ptr);
    QModelIndex nodeToIndex(const VNode *node);
    void selectionChanged(QModelIndexList lst);
    void setSkipSort(bool b) {skipSort_=b;}
    void removeColumn(QString);
    ModelColumn* columns() const;

protected:
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const;

    TableNodeModel* nodeModel_;
    bool skipSort_;
};

#endif
