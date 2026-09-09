/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_ChangeNotifyModel_HPP
#define ecflow_viewer_ChangeNotifyModel_HPP

#include <vector>

#include <QSortFilterProxyModel>

#include "VInfo.hpp"

class VNodeList;

class ChangeNotifyModel : public QAbstractItemModel {
    Q_OBJECT
public:
    explicit ChangeNotifyModel(QObject* parent = nullptr);
    ~ChangeNotifyModel() override;

    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;

    QVariant data(const QModelIndex&, int role = Qt::DisplayRole) const override;
    QVariant headerData(int, Qt::Orientation, int role = Qt::DisplayRole) const override;

    QModelIndex index(int, int, const QModelIndex& parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex&) const override;

    void resetData(VNodeList*);
    bool hasData() const;
    VNodeList* data();
    VInfo_ptr nodeInfo(const QModelIndex&) const;

public Q_SLOTS:
    void slotBeginAppendRow();
    void slotEndAppendRow();
    void slotBeginRemoveRow(int);
    void slotEndRemoveRow(int);
    void slotBeginRemoveRows(int, int);
    void slotEndRemoveRows(int, int);
    void slotBeginReset();
    void slotEndReset();

protected:
    VNodeList* data_{nullptr};
};

#endif /* ecflow_viewer_ChangeNotifyModel_HPP */
