#ifndef TABLENODEMODEL_H
#define TABLENODEMODEL_H

#include <QAbstractItemModel>

#include "AbstractNodeModel.hpp"
#include "ViewNodeInfo.hpp"

class Node;
class ServerFilter;
class ServerHandler;

class TableNodeModel : public AbstractNodeModel
{
public:
   	TableNodeModel(VConfig* config,QObject *parent=0);

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


#endif
