//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef TREENODEMODEL_H
#define TREENODEMODEL_H

#include <QAbstractItemModel>

#include "AbstractNodeModel.hpp"
#include "Node.hpp"
#include "VAttribute.hpp"
#include "Viewer.hpp"
#include "VInfo.hpp"

class AttributeFilter;
class NodeFilter;
class ServerFilter;
class ServerHandler;

class VModelServer;

class TreeNodeModel : public AbstractNodeModel
{
Q_OBJECT

public:
   	TreeNodeModel(VModelData *data,AttributeFilter *atts,IconFilter* icons,QObject *parent=0);

   	int columnCount (const QModelIndex& parent = QModelIndex() ) const;
   	int rowCount (const QModelIndex& parent = QModelIndex() ) const;

   	Qt::ItemFlags flags ( const QModelIndex & index) const;
   	QVariant data (const QModelIndex& , int role = Qt::DisplayRole ) const;
	QVariant headerData(int,Qt::Orientation,int role = Qt::DisplayRole ) const;

   	QModelIndex index (int, int, const QModelIndex& parent = QModelIndex() ) const;
   	QModelIndex parent (const QModelIndex & ) const;

   	VInfo_ptr nodeInfo(const QModelIndex& index);

public Q_SLOTS:
	void slotIconFilterChanged();
	void slotServerAddBegin(int row);
	void slotServerAddEnd();
	void slotServerRemoveBegin(int row);
	void slotServerRemoveEnd();
	void slotDataChanged(VModelServer*);
	void slotNodeChanged(VModelServer*,const VNode*);
	void slotAttributesChanged(VModelServer*,const VNode*);
	void slotBeginAddRemoveAttributes(VModelServer*,const VNode*,int,int);
	void slotEndAddRemoveAttributes(VModelServer*,const VNode*,int,int);
	void slotAddRemoveNodes(VModelServer*,const VNode*,int,int);
	void slotBeginAddRemoveNode(VModelServer*,const VNode*,int,int);
	void slotEndAddRemoveNode(VModelServer*,const VNode*,bool);
	void slotBeginNodeClear(VModelServer* server,const VNode *node);
	void slotEndNodeClear();
	void slotBeginNodeScan(VModelServer* server,const VNode *node,int num);
	void slotEndNodeScan(VModelServer* server,const VNode *node);
	void slotResetBranch(VModelServer*,const VNode*);
	void slotBeginServerScan(VModelServer* server,int);
	void slotEndServerScan(VModelServer* server,int);
	void slotBeginServerClear(VModelServer* server);
	void slotEndServerClear(VModelServer* server);

Q_SIGNALS:
	void filterChanged();
	void clearBegun(const VNode*);
	void scanEnded(const VNode*);

private:
	bool isServer(const QModelIndex & index) const;
	bool isNode(const QModelIndex & index) const;
	bool isAttribute(const QModelIndex & index) const;

	ServerHandler* indexToRealServer(const QModelIndex & index) const;
	VModelServer* indexToServer(const QModelIndex & index) const;
	QModelIndex serverToIndex(VModelServer* server) const;
	QModelIndex serverToIndex(ServerHandler*) const;

	QModelIndex nodeToIndex(const VNode*,int column=0) const;
	QModelIndex nodeToIndex(VModelServer*,const VNode*,int column=0) const;
	VNode* indexToNode( const QModelIndex & index) const;

	QVariant serverData(const QModelIndex& index,int role) const;
	QVariant nodeData(const QModelIndex& index,int role) const;
	QVariant attributesData(const QModelIndex& index,int role) const;

	//Attribute filter
	AttributeFilter* atts_;
};


#endif
