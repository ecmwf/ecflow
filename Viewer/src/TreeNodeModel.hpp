#ifndef TREENODEMODEL_H
#define TREENODEMODEL_H

#include <QAbstractItemModel>

#include "AbstractNodeModel.hpp"
#include "Node.hpp"
#include "VAttribute.hpp"
#include "Viewer.hpp"
#include "ViewNodeInfo.hpp"

class ServerFilter;
class ServerHandler;

class TreeNodeModel : public AbstractNodeModel
{
Q_OBJECT

public:
   	TreeNodeModel(VConfig*,QObject *parent=0);

   	int columnCount (const QModelIndex& parent = QModelIndex() ) const;
   	int rowCount (const QModelIndex& parent = QModelIndex() ) const;

   	Qt::ItemFlags flags ( const QModelIndex & index) const;
   	QVariant data (const QModelIndex& , int role = Qt::DisplayRole ) const;
	QVariant headerData(int,Qt::Orientation,int role = Qt::DisplayRole ) const;

   	QModelIndex index (int, int, const QModelIndex& parent = QModelIndex() ) const;
   	QModelIndex parent (const QModelIndex & ) const;

   	//VFilterGroup Observer
    void notifyConfigChanged(StateFilter*);
    void notifyConfigChanged(AttributeFilter*);
    void notifyConfigChanged(IconFilter*);

signals:
	void filterChanged();

private:
	bool isServer(const QModelIndex & index) const;
	bool isNode(const QModelIndex & index) const;
	bool isAttribute(const QModelIndex & index) const;

	ServerHandler* indexToServer(const QModelIndex & index) const;
	QModelIndex serverToIndex(ServerHandler*) const;
	QModelIndex nodeToIndex(Node*,int column=0) const;
	Node* indexToNode( const QModelIndex & index) const;

	QVariant serverData(const QModelIndex& index,int role) const;
	QVariant nodeData(const QModelIndex& index,int role) const;
	QVariant attributesData(const QModelIndex& index,int role) const;

	void resetStateFilter(bool broadcast);
	bool filterState(node_ptr node,QSet<Node*>& filterSet);
};


#endif
