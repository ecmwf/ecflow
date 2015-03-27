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
public:
   	TableNodeModel(VModelData *data,IconFilter* icons,QObject *parent=0);

	int columnCount (const QModelIndex& parent = QModelIndex() ) const;
   	int rowCount (const QModelIndex& parent = QModelIndex() ) const;

   	QVariant data (const QModelIndex& , int role = Qt::DisplayRole ) const;
	QVariant headerData(int,Qt::Orientation,int role = Qt::DisplayRole ) const;

   	QModelIndex index (int, int, const QModelIndex& parent = QModelIndex() ) const;
   	QModelIndex parent (const QModelIndex & ) const;

   	VInfo_ptr nodeInfo(const QModelIndex&);

protected:
   	bool isServer(const QModelIndex & index) const;
	ServerHandler* indexToRealServer(const QModelIndex & index) const;
	VModelServer* indexToServer(const QModelIndex & index) const;
	QModelIndex serverToIndex(ServerHandler*) const;
	QModelIndex serverToIndex(VModelServer*) const;

	QModelIndex nodeToIndex(VNode*,int column=0) const;
	VNode* indexToNode( const QModelIndex & index) const;

	QVariant serverData(const QModelIndex& index,int role) const;
	QVariant nodeData(const QModelIndex& index,int role) const;

	void resetStateFilter(bool broadcast);
};

#endif
