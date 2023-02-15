//============================================================================
// Copyright 2009- ECMWF.
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

class TableNodeSortModel : public QSortFilterProxyModel {
    Q_OBJECT
public:
    TableNodeSortModel(TableNodeModel*, QObject* parent = nullptr);
    ~TableNodeSortModel() override;

    // From QSortFilterProxyModel:
    // we set the source model in the constructor. So this function should not do anything.
    void setSourceModel(QAbstractItemModel*) override {}

    VInfo_ptr nodeInfo(const QModelIndex&);
    QModelIndex infoToIndex(VInfo_ptr);
    QModelIndex nodeToIndex(const VNode* node);
    void selectionChanged(QModelIndexList lst);
    void setSkipSort(bool b) { skipSort_ = b; }
    void removeColumn(QString);
    ModelColumn* columns() const;
    void sort(int column, Qt::SortOrder order = Qt::AscendingOrder) override;

protected Q_SLOTS:
    void skipSortingBegin();
    void skipSortingEnd();

protected:
    bool lessThan(const QModelIndex& left, const QModelIndex& right) const override;

    TableNodeModel* nodeModel_;
    bool skipSort_;
};

#endif
