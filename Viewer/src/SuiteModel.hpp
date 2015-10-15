#ifndef SUITEMODEL_H
#define SUITEMODEL_H

#include <QAbstractItemModel>
//#include <QSortFilterProxyModel>

#include "SuiteFilterObserver.hpp"

class SuiteFilter;

class SuiteModel : public QAbstractItemModel, public SuiteFilterObserver
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

	void setData(SuiteFilter* filter);
	void reloadData();

   	SuiteFilter* filter() const {return data_;}

   	void notifyChange(SuiteFilter*);
   	void notifyDelete(SuiteFilter*);

protected:
	bool hasData() const;
	void clearData();
	void updateData();

	SuiteFilter* data_;
	SuiteFilter* realData_;
};

#endif
