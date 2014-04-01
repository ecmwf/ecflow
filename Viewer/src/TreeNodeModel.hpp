#ifndef TREENODEMODEL_H
#define TREENODEMODEL_H

#include <QAbstractItemModel>
#include <QSortFilterProxyModel>

#include <vector>

#include "FilterDataObserver.hpp"
#include "ViewNodeInfo.hpp"
#include "AbstractObserver.hpp"

class FilterData;
class Node;
class ServerHandler;


class TreeNodeModel : public QAbstractItemModel, public AbstractObserver
{
Q_OBJECT
public:
   	TreeNodeModel(QObject *parent=0);

   	enum CustomItemRole {FilterRole = Qt::UserRole+1};

   	int columnCount (const QModelIndex& parent = QModelIndex() ) const;
   	int rowCount (const QModelIndex& parent = QModelIndex() ) const;

   	Qt::ItemFlags flags ( const QModelIndex & index) const;
   	QVariant data (const QModelIndex& , int role = Qt::DisplayRole ) const;
	QVariant headerData(int,Qt::Orientation,int role = Qt::DisplayRole ) const;

   	QModelIndex index (int, int, const QModelIndex& parent = QModelIndex() ) const;
   	QModelIndex parent (const QModelIndex & ) const;

	void addServer(ServerHandler *);
	void setRootNode(Node *node);
	void dataIsAboutToChange();
	ViewNodeInfo_ptr nodeInfo(const QModelIndex& index) const;
	void reload();

	//From AbstractObserver
	void update(const Node*, const std::vector<ecf::Aspect::Type>&);
	void update(const Defs*, const std::vector<ecf::Aspect::Type>&)  {};

signals:
	void changed();

protected:
	void init();
	void initObserver(ServerHandler*);
	void clean();
	bool hasData() const;
	bool isServer(const QModelIndex & index) const;
	ServerHandler* indexToServer(const QModelIndex & index) const;
	QModelIndex serverToIndex(ServerHandler*) const;
	QModelIndex nodeToIndex(Node*,int column=0) const;
	Node* indexToNode( const QModelIndex & index) const;

	QVariant serverData(const QModelIndex& index,int role) const;
	QVariant nodeData(const QModelIndex& index,int role) const;
	Node *rootNode(ServerHandler*) const;

	QList<ServerHandler*> servers_;
	QMap<ServerHandler*,Node*> rootNodes_;

};

class TreeNodeFilterModel : public QSortFilterProxyModel, public FilterDataObserver
{
public:
	TreeNodeFilterModel(FilterData*,QObject *parent=0);
	~TreeNodeFilterModel();

	bool filterAcceptsRow(int,const QModelIndex &) const;

	//Observer method
	void notifyFilterChanged();

protected:
	FilterData *filterData_;
};





#endif
