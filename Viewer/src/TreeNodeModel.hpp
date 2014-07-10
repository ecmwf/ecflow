#ifndef TREENODEMODEL_H
#define TREENODEMODEL_H

#include <QAbstractItemModel>
#include <QSortFilterProxyModel>

#include <vector>

#include "AbstractNodeModel.hpp"
#include "ViewFilterObserver.hpp"
#include "ViewNodeInfo.hpp"

class Node;
class ServerFilter;
class ServerHandler;
class ViewFilter;

class TreeNodeModel : public AbstractNodeModel
{
public:
   	TreeNodeModel(ServerFilter*,QObject *parent=0);

   	int columnCount (const QModelIndex& parent = QModelIndex() ) const;
   	int rowCount (const QModelIndex& parent = QModelIndex() ) const;

   	Qt::ItemFlags flags ( const QModelIndex & index) const;
   	QVariant data (const QModelIndex& , int role = Qt::DisplayRole ) const;
	QVariant headerData(int,Qt::Orientation,int role = Qt::DisplayRole ) const;

   	QModelIndex index (int, int, const QModelIndex& parent = QModelIndex() ) const;
   	QModelIndex parent (const QModelIndex & ) const;

   void setFilter(const std::set<DState::State>& ns);

protected:
	bool isServer(const QModelIndex & index) const;
	bool isNode(const QModelIndex & index) const;
	bool isAttribute(const QModelIndex & index) const;

	ServerHandler* indexToServer(const QModelIndex & index) const;
	QModelIndex serverToIndex(ServerHandler*) const;
	QModelIndex nodeToIndex(Node*,int column=0) const;
	Node* indexToNode( const QModelIndex & index) const;

	int attributesNum(Node*) const;

	QVariant serverData(const QModelIndex& index,int role) const;
	QVariant nodeData(const QModelIndex& index,int role) const;

	bool filter(node_ptr node,const std::set<DState::State>& ns,QList<Node*>& filterVec);
};


class TreeNodeFilterModel : public QSortFilterProxyModel, public ViewFilterObserver
{
public:
	TreeNodeFilterModel(ViewFilter*,QObject *parent=0);
	~TreeNodeFilterModel();

	bool filterAcceptsRow(int,const QModelIndex &) const;

	//Observer method
	void notifyFilterChanged();

protected:
	ViewFilter *viewFilter_;
};





#endif
