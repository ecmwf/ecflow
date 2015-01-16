#ifndef ABSTRACTNODEMODEL_H
#define ABSTRACTNODEMODEL_H

#include <QAbstractItemModel>
#include <QSet>

#include <set>
#include <vector>

#include "Aspect.hpp"
#include "NodeObserver.hpp"
#include "VConfig.hpp"
#include "VInfo.hpp"

class Node;

class ServerHandler;
class VFilter;

class AbstractNodeModel;

class NodeModelServerItem
{
friend class NodeModelServers;

public:
    NodeModelServerItem(ServerHandler *server) : server_(server), rootNode_(NULL), nodeNum_(-1) {}
    bool isFiltered(Node*) const;

    void resetFilter(VFilter* stateFilter);
    int nodeNum() const;

protected:
    bool filterState(node_ptr node,VFilter* stateFilter);

    ServerHandler *server_;
	QSet<Node*> nodeFilter_;
	Node* rootNode_;
	mutable int nodeNum_;

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
	void resetFilter(VFilter* stateFilter);

	QList<NodeModelServerItem> items_;
};


class AbstractNodeModel : public QAbstractItemModel, public VConfigObserver, public NodeObserver
{
	Q_OBJECT

public:
	AbstractNodeModel(VConfig*,QObject *parent=0);
   	virtual ~AbstractNodeModel();

   	enum CustomItemRole {FilterRole = Qt::UserRole+1, IconRole = Qt::UserRole+2,
   		                 ServerRole = Qt::UserRole+3};

	void addServer(ServerHandler *);
	void setRootNode(Node *node);
	void dataIsAboutToChange();
	virtual VInfo_ptr nodeInfo(const QModelIndex& index);
	void reload();
	void active(bool);
	bool active() const {return active_;}

	virtual QModelIndex infoToIndex(VInfo_ptr,int column=0) const;

	//From ConfigObserver
	void notifyConfigChanged(ServerFilter*);
	void notifyConfigChanged(StateFilter*) {};
	void notifyConfigChanged(AttributeFilter*) {};
	void notifyConfigChanged(IconFilter*) {};

	//From NodeObserver
	void notifyNodeChanged(const Node*, const std::vector<ecf::Aspect::Type>&);

Q_SIGNALS:
	void changed();
	void filterChanged();

protected:
	void init();
	void clean();
	bool hasData() const;

	virtual void resetStateFilter(bool broadcast);

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
	bool active_;
};

#endif
