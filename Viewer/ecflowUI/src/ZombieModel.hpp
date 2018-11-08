//============================================================================
// Copyright 2009-2018 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef ZOMBIEMODEL_H
#define ZOMBIEMODEL_H

#include <QAbstractItemModel>
#include <QSortFilterProxyModel>

#include <vector>

#include "Zombie.hpp"

class ModelColumn;

class ZombieModel : public QAbstractItemModel
{
public:
   	explicit ZombieModel(QObject *parent=nullptr);
   	~ZombieModel() override;

   	int columnCount (const QModelIndex& parent = QModelIndex() ) const override;
   	int rowCount (const QModelIndex& parent = QModelIndex() ) const override;

   	Qt::ItemFlags flags ( const QModelIndex & index) const override;
   	QVariant data (const QModelIndex& , int role = Qt::DisplayRole ) const override;
   	QVariant headerData(int,Qt::Orientation,int role = Qt::DisplayRole ) const override;

   	QModelIndex index (int, int, const QModelIndex& parent = QModelIndex() ) const override;
   	QModelIndex parent (const QModelIndex & ) const override;

	void setData(const std::vector<Zombie>&);
	bool updateData(const std::vector<Zombie>&);
    void clearData();
    bool hasData() const;
    Zombie indexToZombie(const QModelIndex&) const;

protected:
	std::vector<Zombie> data_;
	ModelColumn* columns_{nullptr};
};


#endif
