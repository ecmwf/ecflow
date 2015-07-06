#ifndef SUITEMODEL_H
#define SUITEMODEL_H

#include <QAbstractItemModel>
#include <QSortFilterProxyModel>

#include <vector>

#include "NodeObserver.hpp"

class SuiteFilter;

class SuiteModel : public QAbstractItemModel
{
public:
   	explicit SuiteModel(QObject *parent=0);
   	~SuiteModel();

   	int columnCount (const QModelIndex& parent = QModelIndex() ) const;
   	int rowCount (const QModelIndex& parent = QModelIndex() ) const;

   	Qt::ItemFlags flags ( const QModelIndex & index) const;
   	QVariant data (const QModelIndex& , int role = Qt::DisplayRole ) const;
   	bool setData(const QModelIndex & index, const QVariant & value, int role );
   	QVariant headerData(int,Qt::Orientation,int role = Qt::DisplayRole ) const;

   	QModelIndex index (int, int, const QModelIndex& parent = QModelIndex() ) const;
   	QModelIndex parent (const QModelIndex & ) const;

	void reloadData();
	void setData(SuiteFilter* filter);
   	SuiteFilter* filter() const {return data_;}

protected:
	bool hasData() const;

	SuiteFilter* data_;
};

#endif
