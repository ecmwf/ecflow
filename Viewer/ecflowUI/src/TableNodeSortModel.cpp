/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#include "TableNodeSortModel.hpp"

#include <QDebug>

#include "ModelColumn.hpp"
#include "TableNodeModel.hpp"

TableNodeSortModel::TableNodeSortModel(TableNodeModel* nodeModel, QObject* parent)
    : QSortFilterProxyModel(parent),
      nodeModel_(nodeModel),
      skipSort_(false) {
    Q_ASSERT(nodeModel_);
    connect(nodeModel_, SIGNAL(skipSortingBegin()), this, SLOT(skipSortingBegin()));
    connect(nodeModel_, SIGNAL(skipSortingEnd()), this, SLOT(skipSortingEnd()));

    QSortFilterProxyModel::setSourceModel(nodeModel_);

    setDynamicSortFilter(false);
}

TableNodeSortModel::~TableNodeSortModel() = default;

VInfo_ptr TableNodeSortModel::nodeInfo(const QModelIndex& index) {
    return nodeModel_->nodeInfo(mapToSource(index));
}

QModelIndex TableNodeSortModel::infoToIndex(VInfo_ptr info) {
    return mapFromSource(nodeModel_->infoToIndex(info));
}

QModelIndex TableNodeSortModel::nodeToIndex(const VNode* node) {
    return mapFromSource(nodeModel_->nodeToIndex(node));
}

void TableNodeSortModel::selectionChanged(QModelIndexList lst) {
    QModelIndexList lstm;
    Q_FOREACH (QModelIndex idx, lst) {
        lstm << mapToSource(idx);
    }

    nodeModel_->selectionChanged(lstm);
}

bool TableNodeSortModel::lessThan(const QModelIndex& left, const QModelIndex& right) const {
    if (skipSort_) {
        return true;
    }

    auto id = static_cast<TableNodeModel::ColumnType>(left.column());

    if (id == TableNodeModel::PathColumn) {
        return left.row() < right.row();
    }

    else if (id == TableNodeModel::MeterColumn) {
        return left.data(AbstractNodeModel::SortRole).toInt() < right.data(AbstractNodeModel::SortRole).toInt();
    }

    else if (id == TableNodeModel::StatusChangeColumn) {
        return left.data(AbstractNodeModel::SortRole).toUInt() < right.data(AbstractNodeModel::SortRole).toUInt();
    }

    QVariant leftData  = nodeModel_->data(left);
    QVariant rightData = nodeModel_->data(right);

    return leftData.toString() < rightData.toString();
}

void TableNodeSortModel::removeColumn(QString name) {
    nodeModel_->removeColumn(name);
}

ModelColumn* TableNodeSortModel::columns() const {
    return nodeModel_->columns();
}

void TableNodeSortModel::sort(int column, Qt::SortOrder order) {
    if (!skipSort_) {
        QSortFilterProxyModel::sort(column, order);
    }
}

void TableNodeSortModel::skipSortingBegin() {
    setSkipSort(true);
}

void TableNodeSortModel::skipSortingEnd() {
    setSkipSort(false);
    sort(sortColumn(), sortOrder());
}
