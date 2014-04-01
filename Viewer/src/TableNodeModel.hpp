#ifndef TABLENODEMODEL_H
#define TABLENODEMODEL_H

#include <QAbstractItemModel>
#include <QSortFilterProxyModel>

#include <vector>

#include "FilterDataObserver.hpp"
#include "ViewNodeInfo.hpp"
#include "AbstractObserver.hpp"

class FilterData;
class Node;
class ServerHandler;


class TableNodeModel : public QAbstractItemModel, public AbstractObserver
{

public:
   	TableNodeModel(QObject *parent=0);

	enum CustomItemRole {FilterRole = Qt::UserRole+1};

	int columnCount (const QModelIndex& parent = QModelIndex() ) const;
   	int rowCount (const QModelIndex& parent = QModelIndex() ) const;

   	QVariant data (const QModelIndex& , int role = Qt::DisplayRole ) const;
	QVariant headerData(int,Qt::Orientation,int role = Qt::DisplayRole ) const;

   	QModelIndex index (int, int, const QModelIndex& parent = QModelIndex() ) const;
   	QModelIndex parent (const QModelIndex & ) const;

	void addServer(ServerHandler *);
	void dataIsAboutToChange();
	ViewNodeInfo_ptr nodeInfo(const QModelIndex& index) const;

	//From AbstractObserver
	void update(const Node*, const std::vector<ecf::Aspect::Type>&);
	void update(const Defs*, const std::vector<ecf::Aspect::Type>&)  {};


protected:
	void initObserver(ServerHandler* server);
    bool hasData() const;
	bool isServer(const QModelIndex & index) const;
	ServerHandler* indexToServer(const QModelIndex & index) const;
	QModelIndex serverToIndex(ServerHandler*) const;
	QModelIndex nodeToIndex(Node*,int) const;
	Node* indexToNode( const QModelIndex & index) const;

	QVariant serverData(const QModelIndex& index,int role) const;
	QVariant nodeData(const QModelIndex& index,int role) const;
	Node *rootNode(ServerHandler*) const;

	QList<ServerHandler*> servers_;
	QMap<ServerHandler*,Node*> rootNodes_;
};


class TableNodeFilterModel : public QSortFilterProxyModel, public FilterDataObserver
{
public:
	TableNodeFilterModel(FilterData*,QObject *parent=0);
	~TableNodeFilterModel();

	bool filterAcceptsRow(int,const QModelIndex &) const;

	//Observer method
	void notifyFilterChanged();

protected:
	FilterData *filterData_;
};






#endif
