#ifndef TABLENODEMODEL_H
#define TABLENODEMODEL_H

#include <QAbstractItemModel>
#include <QSortFilterProxyModel>

#include <vector>

#include "AbstractNodeModel.hpp"
#include "ViewFilterObserver.hpp"
#include "ViewNodeInfo.hpp"

class ViewFilter;
class Node;
class ServerFilter;
class ServerHandler;

class TableNodeModel : public AbstractNodeModel
{
public:
   	TableNodeModel(ServerFilter* serverFilter,QObject *parent=0);

	int columnCount (const QModelIndex& parent = QModelIndex() ) const;
   	int rowCount (const QModelIndex& parent = QModelIndex() ) const;

   	QVariant data (const QModelIndex& , int role = Qt::DisplayRole ) const;
	QVariant headerData(int,Qt::Orientation,int role = Qt::DisplayRole ) const;

   	QModelIndex index (int, int, const QModelIndex& parent = QModelIndex() ) const;
   	QModelIndex parent (const QModelIndex & ) const;

protected:
	bool isServer(const QModelIndex & index) const;
	ServerHandler* indexToServer(const QModelIndex & index) const;
	QModelIndex serverToIndex(ServerHandler*) const;
	QModelIndex nodeToIndex(Node*,int column=0) const;
	Node* indexToNode( const QModelIndex & index) const;

	QVariant serverData(const QModelIndex& index,int role) const;
	QVariant nodeData(const QModelIndex& index,int role) const;
};


class TableNodeFilterModel : public QSortFilterProxyModel, public ViewFilterObserver
{
public:
	TableNodeFilterModel(ViewFilter*,QObject *parent=0);
	~TableNodeFilterModel();

	bool filterAcceptsRow(int,const QModelIndex &) const;

	//Observer method
	void notifyFilterChanged();

protected:
	ViewFilter *viewFilter_;
};






#endif
