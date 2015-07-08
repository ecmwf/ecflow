//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef TABLENODEMODEL_H
#define TABLENODEMODEL_H

#include <QAbstractItemModel>

#include "AbstractNodeModel.hpp"
#include "VInfo.hpp"

class Node;
class NodeFilter;
class ServerFilter;
class ServerHandler;

class TableNodeModel : public AbstractNodeModel
{
Q_OBJECT

public:
   	TableNodeModel(VModelData *data,IconFilter* icons,QObject *parent=0);

	int columnCount (const QModelIndex& parent = QModelIndex() ) const;
   	int rowCount (const QModelIndex& parent = QModelIndex() ) const;

   	QVariant data (const QModelIndex& , int role = Qt::DisplayRole ) const;
	QVariant headerData(int,Qt::Orientation,int role = Qt::DisplayRole ) const;

   	QModelIndex index (int, int, const QModelIndex& parent = QModelIndex() ) const;
   	QModelIndex parent (const QModelIndex & ) const;

   	VInfo_ptr nodeInfo(const QModelIndex&);

public Q_SLOTS:
	void slotNodeChanged(VModelServer* server,const VNode* node);
	void slotBeginServerScan(VModelServer* server,int num);
	void slotEndServerScan(VModelServer* server,int num);
	void slotBeginServerClear(VModelServer* server,int num);
   	void slotEndServerClear(VModelServer* server,int num);

protected:
   	bool isServer(const QModelIndex & index) const {return false;}
	ServerHandler* indexToRealServer(const QModelIndex & index) const {return NULL;}
	VModelServer* indexToServer(const QModelIndex & index) const {return NULL;}
	QModelIndex serverToIndex(ServerHandler*) const {return QModelIndex();}
	//QModelIndex serverToIndex(VModelServer*) const;

   	QModelIndex nodeToIndex(VModelServer* server,const VNode* node, int column) const;
   	QModelIndex nodeToIndex(const VNode*,int column=0) const;
	VNode* indexToNode( const QModelIndex & index) const;

	QVariant nodeData(const QModelIndex& index,int role) const;
	QVariant serverData(const QModelIndex& index,int role) const {return QVariant();}
};

#endif
