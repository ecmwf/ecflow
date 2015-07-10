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
class NodeFilterDef;
class ServerFilter;
class ServerHandler;
class VTableModelData;

class TableNodeModel : public AbstractNodeModel
{
Q_OBJECT

public:
   	TableNodeModel(ServerFilter* serverFilter,NodeFilterDef* filterDef,QObject *parent=0);

	int columnCount (const QModelIndex& parent = QModelIndex() ) const;
   	int rowCount (const QModelIndex& parent = QModelIndex() ) const;

   	QVariant data (const QModelIndex& , int role = Qt::DisplayRole ) const;
	QVariant headerData(int,Qt::Orientation,int role = Qt::DisplayRole ) const;

   	QModelIndex index (int, int, const QModelIndex& parent = QModelIndex() ) const;
   	QModelIndex parent (const QModelIndex & ) const;

   	VInfo_ptr nodeInfo(const QModelIndex&);

   	VModelData* data() const;

public Q_SLOTS:
   	void slotServerAddBegin(int row);
   	void slotServerAddEnd();
   	void slotServerRemoveBegin(int row);
   	void slotServerRemoveEnd();

   	void slotDataChanged(VModelServer*) {}
   	void slotNodeChanged(VModelServer*,const VNode*);
   	void slotAttributesChanged(VModelServer*,const VNode*) {};
   	void slotBeginAddRemoveAttributes(VModelServer*,const VNode*,int,int) {};
   	void slotEndAddRemoveAttributes(VModelServer*,const VNode*,int,int) {};

   	void slotBeginServerScan(VModelServer* server,int);
   	void slotEndServerScan(VModelServer* server,int);
   	void slotBeginServerClear(VModelServer* server,int);
   	void slotEndServerClear(VModelServer* server,int);

protected:
   	bool isServer(const QModelIndex & index) const {return false;}
	ServerHandler* indexToRealServer(const QModelIndex & index) const {return NULL;}
	VModelServer* indexToServer(const QModelIndex & index) const {return NULL;}
	QModelIndex serverToIndex(ServerHandler*) const {return QModelIndex();}

   	QModelIndex nodeToIndex(VModelServer* server,const VNode* node, int column) const;
   	QModelIndex nodeToIndex(const VNode*,int column=0) const;
	VNode* indexToNode( const QModelIndex & index) const;

	QVariant nodeData(const QModelIndex& index,int role) const;

	VTableModelData* data_;
};

#endif
