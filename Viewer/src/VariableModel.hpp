#ifndef VARIABLEMODEL_H
#define VARIABLEMODEL_H

#include <QAbstractItemModel>
#include <QSortFilterProxyModel>

#include <vector>

#include "NodeObserver.hpp"
#include "VInfo.hpp"

class Node;
class VariableModelData;
class VariableModelDataHandler;
class VariableSortModel;

class VariableModel : public QAbstractItemModel
{
Q_OBJECT

friend class VariableSortModel;

public:
   	VariableModel(VariableModelDataHandler* data,QObject *parent=0);

   	int columnCount (const QModelIndex& parent = QModelIndex() ) const;
   	int rowCount (const QModelIndex& parent = QModelIndex() ) const;

   	Qt::ItemFlags flags ( const QModelIndex & index) const;
   	QVariant data (const QModelIndex& , int role = Qt::DisplayRole ) const;
	QVariant headerData(int,Qt::Orientation,int role = Qt::DisplayRole ) const;

   	QModelIndex index (int, int, const QModelIndex& parent = QModelIndex() ) const;
   	QModelIndex parent (const QModelIndex & ) const;

	bool data(const QModelIndex& index, QString& name,QString& value) const;
	bool setData(const QModelIndex& index,QString name,QString value);

	void nodeChanged(const Node*, const std::vector<ecf::Aspect::Type>&);

public Q_SLOTS:
	void slotReloadBegin();
	void slotReloadEnd();

protected:
	bool hasData() const;

	int indexToLevel(const QModelIndex&) const;
	bool isVariable(const QModelIndex & index) const;
	/*
	bool isServer(const QModelIndex & index) const;
	bool isNode(const QModelIndex & index) const;
	bool isVariable(const QModelIndex & index) const;
	ServerHandler* indexToServer(const QModelIndex & index) const;
	Node* indexToNode( const QModelIndex & index) const;

	QModelIndex nodeToIndex(Node *node) const;*/

	VariableModelDataHandler* data_;
};


//Filter and sorts the variables

class VariableSortModel : public QSortFilterProxyModel
{
public:
	VariableSortModel(VariableModel*,QObject *parent=0);
	~VariableSortModel() {};

	bool lessThan(const QModelIndex &left, const QModelIndex &right) const;
	bool filterAcceptsRow(int,const QModelIndex &) const;

	//From QSortFilterProxyModel:
	//we set the source model in the constructor. So this function should not do anything.
	void setSourceModel(QAbstractItemModel*) {};

protected:
	VariableModel* varModel_;
};


#endif
