#ifndef VARIABLEMODEL_H
#define VARIABLEMODEL_H

#include <QAbstractItemModel>

#include <vector>

#include "ViewNodeInfo.hpp"

class Node;
class ServerHandler;

class VariableModel : public QAbstractItemModel
{

public:
   	VariableModel(QObject *parent=0);

   	int columnCount (const QModelIndex& parent = QModelIndex() ) const;
   	int rowCount (const QModelIndex& parent = QModelIndex() ) const;

   	Qt::ItemFlags flags ( const QModelIndex & index) const;
   	QVariant data (const QModelIndex& , int role = Qt::DisplayRole ) const;
	QVariant headerData(int,Qt::Orientation,int role = Qt::DisplayRole ) const;

   	QModelIndex index (int, int, const QModelIndex& parent = QModelIndex() ) const;
   	QModelIndex parent (const QModelIndex & ) const;

	void setData(ViewNodeInfo_ptr);

protected:

	bool hasData() const;
	int indexToLevel(const QModelIndex&) const;
	bool isServer(const QModelIndex & index) const;
	bool isNode(const QModelIndex & index) const;
	ServerHandler* indexToServer(const QModelIndex & index) const;
	Node* indexToNode( const QModelIndex & index) const;

	QVariant serverData(const QModelIndex& index,int role) const;
	QVariant nodeData(const QModelIndex& index,int role) const;
	QVariant variableData(ServerHandler*,const QModelIndex& index,int role) const;
	QVariant variableData(Node*,const QModelIndex& index,int role) const;

	QList<Node*> nodes_;
	ServerHandler *server_;

};

#endif
