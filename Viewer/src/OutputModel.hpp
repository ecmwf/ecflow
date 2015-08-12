#ifndef OUTPUTMODEL_H
#define OUTPUTMODEL_H

#include <QAbstractItemModel>
#include <QSortFilterProxyModel>

#include "NodeObserver.hpp"
#include "VDir.hpp"
#include "VInfo.hpp"

class OutputModel : public QAbstractItemModel
{
public:
	explicit OutputModel(QObject *parent=0);

   	void setData(VDir_ptr dir);
   	void clearData();

   	int columnCount (const QModelIndex& parent = QModelIndex() ) const;
   	int rowCount (const QModelIndex& parent = QModelIndex() ) const;

   	//Qt::ItemFlags flags ( const QModelIndex & index) const;
   	QVariant data (const QModelIndex& , int role = Qt::DisplayRole ) const;
	QVariant headerData(int,Qt::Orientation,int role = Qt::DisplayRole ) const;

   	QModelIndex index (int, int, const QModelIndex& parent = QModelIndex() ) const;
   	QModelIndex parent (const QModelIndex & ) const;

   	std::string fullName(const QModelIndex& index) const;

protected:
   	bool hasData() const;
   	QString formatSize(unsigned int size) const;
   	QString formatDate(const std::time_t&) const;
   	QString formatAgo(const std::time_t& t) const;

   	VDir_ptr dir_;
};

//Filters and sorts the output
class OutputSortModel : public QSortFilterProxyModel
{
public:
	explicit OutputSortModel(QObject *parent=0);
	~OutputSortModel() {};

	bool lessThan(const QModelIndex &left, const QModelIndex &right) const;
	bool filterAcceptsRow(int,const QModelIndex &) const;

	QModelIndex fullNameToIndex(const std::string& fullName);
};


#endif
