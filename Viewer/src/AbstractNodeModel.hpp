#ifndef ABSTRACTNODEMODEL_H
#define ABSTRACTNODEMODEL_H

#include <QAbstractItemModel>
#include <QSet>

#include <set>
#include <vector>

#include "Aspect.hpp"
#include "VConfig.hpp"
#include "ViewNodeInfo.hpp"

class Node;

class ServerHandler;
class VFilter;

class AbstractNodeModel;

class NodeModelServerItem
{
friend class NodeModelServers;

public:
    NodeModelServerItem(ServerHandler *server) : server_(server), rootNode_(NULL) {}
    bool isFiltered(Node*) const;

protected:
	ServerHandler *server_;
	QSet<Node*> nodeFilter_;
	Node* rootNode_;

	QList<NodeModelServerItem*> children_;
};


class NodeModelServers
{
public:
	ServerHandler* server(int) const;
	ServerHandler* server(void*) const;
	void add(ServerHandler *);
	int index(ServerHandler* s) const;
	int count() const {return items_.count();}
	void clear() {items_.clear();}
	void clearFilter();
	void nodeFilter(int i,QSet<Node*>);
	bool isFiltered(Node *node) const;

protected:
	QList<NodeModelServerItem> items_;
};


class AbstractNodeModel : public QAbstractItemModel, public VConfigObserver
{
	Q_OBJECT

public:
	AbstractNodeModel(VConfig*,QObject *parent=0);
   	virtual ~AbstractNodeModel();

   	enum CustomItemRole {FilterRole = Qt::UserRole+1};

	void addServer(ServerHandler *);
	void setRootNode(Node *node);
	void dataIsAboutToChange();
	ViewNodeInfo_ptr nodeInfo(const QModelIndex& index) const;
	void reload();

	//From ConfigObserver
	void notifyConfigChanged(ServerFilter*);
	void notifyConfigChanged(StateFilter*) {};
	void notifyConfigChanged(AttributeFilter*) {};
	void notifyConfigChanged(IconFilter*) {};

public slots:
	void slotNodeChanged(const Node*, QList<ecf::Aspect::Type>);

signals:
	void changed();
	void filterChanged();

protected:
	void init();
	void clean();
	bool hasData() const;

	virtual bool isServer(const QModelIndex & index) const=0;
	virtual ServerHandler* indexToServer(const QModelIndex & index) const=0;
	virtual QModelIndex serverToIndex(ServerHandler*) const=0;
	virtual QModelIndex nodeToIndex(Node*,int column=0) const=0;
	virtual Node* indexToNode( const QModelIndex & index) const=0;

	virtual QVariant serverData(const QModelIndex& index,int role) const=0;
	virtual QVariant nodeData(const QModelIndex& index,int role) const=0;

	Node *rootNode(ServerHandler*) const;

	//virtual bool filter(node_ptr node,const std::set<DState::State>& ns,QSet<Node*> filterSet) {return true;}

	NodeModelServers servers_;
	VConfig* config_;
};

#endif
