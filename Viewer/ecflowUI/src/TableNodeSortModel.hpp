/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_TableNodeSortModel_HPP
#define ecflow_viewer_TableNodeSortModel_HPP

#include <QSortFilterProxyModel>

#include "VInfo.hpp"

class TableNodeModel;
class NodeFilterDef;
class ModelColumn;

class TableNodeSortModel : public QSortFilterProxyModel {
    Q_OBJECT
public:
    explicit TableNodeSortModel(TableNodeModel*, QObject* parent = nullptr);
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

#endif /* ecflow_viewer_TableNodeSortModel_HPP */
