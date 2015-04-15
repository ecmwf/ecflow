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
   	OutputModel(QObject *parent=0);

   	void setData(VDir_ptr dir);

   	int columnCount (const QModelIndex& parent = QModelIndex() ) const;
   	int rowCount (const QModelIndex& parent = QModelIndex() ) const;

   	//Qt::ItemFlags flags ( const QModelIndex & index) const;
   	QVariant data (const QModelIndex& , int role = Qt::DisplayRole ) const;
	QVariant headerData(int,Qt::Orientation,int role = Qt::DisplayRole ) const;

   	QModelIndex index (int, int, const QModelIndex& parent = QModelIndex() ) const;
   	QModelIndex parent (const QModelIndex & ) const;

protected:
   	bool hasData() const;
   	QString formatSize(unsigned int size) const;
   	QString formatDate(const std::time_t&) const;

   	VDir_ptr dir_;
};


//Filter and sorts the variables

/*class VariableSortModel : public QSortFilterProxyModel
{
public:
	enum MatchMode {FilterMode,SearchMode};

	VariableSortModel(VariableModel*,QObject *parent=0);
	~VariableSortModel() {};

	MatchMode matchMode() const {return matchMode_;}
	void setMatchMode(MatchMode mode);
	void setMatchText(QString text);

	bool lessThan(const QModelIndex &left, const QModelIndex &right) const;
	bool filterAcceptsRow(int,const QModelIndex &) const;

	//From QSortFilterProxyModel:
	//we set the source model in the constructor. So this function should not do anything.
	void setSourceModel(QAbstractItemModel*) {};
    QVariant data (const QModelIndex& , int role = Qt::DisplayRole ) const;

    QModelIndexList match(const QModelIndex& start,int role,const QVariant& value,int hits = 1, Qt::MatchFlags flags = Qt::MatchFlags( Qt::MatchStartsWith | Qt::MatchWrap )) const;

protected:
    void match(QString text);

	VariableModel* varModel_;

	MatchMode matchMode_;
	mutable QString matchText_;
    mutable QModelIndexList matchLst_;

    QMap<QString,int> nameCnt_;
    bool ignoreDuplicateNames_; //Ignore duplicate names across ancestors
};*/


#endif
