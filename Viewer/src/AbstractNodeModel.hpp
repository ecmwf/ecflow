#ifndef ABSTRACTNODEMODEL_H
#define ABSTRACTNODEMODEL_H

#include <QAbstractItemModel>
#include <QSortFilterProxyModel>

#include <vector>

#include "Aspect.hpp"
#include "ServerFilterObserver.hpp"
#include "ViewFilterObserver.hpp"
#include "ViewNodeInfo.hpp"

class ViewFilter;
class Node;
class ServerFilter;
class ServerHandler;

class AbstractNodeModel : public QAbstractItemModel, public ServerFilterObserver
{
	Q_OBJECT

public:
	AbstractNodeModel(ServerFilter*,QObject *parent=0);
   	virtual ~AbstractNodeModel();

   	enum CustomItemRole {FilterRole = Qt::UserRole+1};

	void addServer(ServerHandler *);
	void setRootNode(Node *node);
	void dataIsAboutToChange();
	ViewNodeInfo_ptr nodeInfo(const QModelIndex& index) const;
	void reload();

	//From ServerFilterObserver
	void notifyServerFilterChanged();

public slots:
	void slotNodeChanged(const Node*, const std::vector<ecf::Aspect::Type>&);

signals:
	void changed();

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

	QList<ServerHandler*> servers_;
	QMap<ServerHandler*,Node*> rootNodes_;
	ServerFilter* serverFilter_;

};

#endif
