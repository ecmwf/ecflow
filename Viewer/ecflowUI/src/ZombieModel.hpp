/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_ZombieModel_HPP
#define ecflow_viewer_ZombieModel_HPP

#include <vector>

#include <QAbstractItemModel>
#include <QSortFilterProxyModel>

#include "ecflow/attribute/Zombie.hpp"

class ModelColumn;

class ZombieModel : public QAbstractItemModel {
public:
    explicit ZombieModel(QObject* parent = nullptr);
    ~ZombieModel() override;

    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;

    Qt::ItemFlags flags(const QModelIndex& index) const override;
    QVariant data(const QModelIndex&, int role = Qt::DisplayRole) const override;
    QVariant headerData(int, Qt::Orientation, int role = Qt::DisplayRole) const override;

    QModelIndex index(int, int, const QModelIndex& parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex&) const override;

    void resetData(const std::vector<Zombie>&);
    bool updateData(const std::vector<Zombie>&);
    void clearData();
    bool hasData() const;
    Zombie indexToZombie(const QModelIndex&) const;

protected:
    std::vector<Zombie> data_;
    ModelColumn* columns_{nullptr};
};

#endif /* ecflow_viewer_ZombieModel_HPP */
