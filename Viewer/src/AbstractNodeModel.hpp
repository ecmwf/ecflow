//============================================================================
// Copyright 2015 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef ABSTRACTNODEMODEL_H
#define ABSTRACTNODEMODEL_H

#include <QAbstractItemModel>

#include "Aspect.hpp"
#include "NodeObserver.hpp"
#include "VConfig.hpp"
#include "VInfo.hpp"

class Node;

class NodeFilter;
class NodeModelDataHandler;
class ServerHandler;
class VFilter;

class AbstractNodeModel;

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

	//Creates a filter object specific for the concrete model type
	virtual NodeFilter* makeFilter()=0;

	virtual void resetStateFilter(bool broadcast)=0;

	virtual bool isServer(const QModelIndex & index) const=0;
	virtual ServerHandler* indexToServer(const QModelIndex & index) const=0;
	virtual QModelIndex serverToIndex(ServerHandler*) const=0;
	virtual QModelIndex nodeToIndex(Node*,int column=0) const=0;
	virtual Node* indexToNode( const QModelIndex & index) const=0;

	virtual QVariant serverData(const QModelIndex& index,int role) const=0;
	virtual QVariant nodeData(const QModelIndex& index,int role) const=0;

	Node *rootNode(ServerHandler*) const;

	NodeModelDataHandler* servers_;
	VConfig* config_;
	bool active_;
};

#endif
