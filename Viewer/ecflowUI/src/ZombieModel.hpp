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
   	~ZombieModel();

   	int columnCount (const QModelIndex& parent = QModelIndex() ) const;
   	int rowCount (const QModelIndex& parent = QModelIndex() ) const;

   	Qt::ItemFlags flags ( const QModelIndex & index) const;
   	QVariant data (const QModelIndex& , int role = Qt::DisplayRole ) const;
   	QVariant headerData(int,Qt::Orientation,int role = Qt::DisplayRole ) const;

   	QModelIndex index (int, int, const QModelIndex& parent = QModelIndex() ) const;
   	QModelIndex parent (const QModelIndex & ) const;

	void setData(const std::vector<Zombie>&);
	bool updateData(const std::vector<Zombie>&);
    void clearData();
    bool hasData() const;
    Zombie indexToZombie(const QModelIndex&) const;

protected:
	std::vector<Zombie> data_;
	ModelColumn* columns_;
};


#endif
