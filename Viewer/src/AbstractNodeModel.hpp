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
#include "VInfo.hpp"

class Node;

class IconFilter;
class VModelData;
class VModelServer;
class VParamSet;

class AbstractNodeModel;

class AbstractNodeModel : public QAbstractItemModel
{
	Q_OBJECT

public:
	AbstractNodeModel(VModelData *data,IconFilter* icons,QObject *parent=0);
   	virtual ~AbstractNodeModel();

   	enum CustomItemRole {FilterRole = Qt::UserRole+1, IconRole = Qt::UserRole+2,
   		                 ServerRole = Qt::UserRole+3, NodeNumRole = Qt::UserRole+4};

	void dataIsAboutToChange();
	virtual VInfo_ptr nodeInfo(const QModelIndex& index)=0;
	void reload();
	void active(bool);
	bool active() const {return active_;}

	virtual QModelIndex infoToIndex(VInfo_ptr,int column=0) const;

Q_SIGNALS:
	void changed();
	void filterChanged();

protected:
	void init();
	void clean();
	bool hasData() const;

	virtual void resetStateFilter(bool broadcast) {};

	virtual bool isServer(const QModelIndex & index) const=0;
	virtual ServerHandler* indexToRealServer(const QModelIndex & index) const=0;
	virtual VModelServer* indexToServer(const QModelIndex & index) const=0;
	virtual QModelIndex serverToIndex(ServerHandler*) const=0;

	virtual QModelIndex nodeToIndex(VNode*,int column=0) const=0;
	virtual VNode* indexToNode( const QModelIndex & index) const=0;

	virtual QVariant serverData(const QModelIndex& index,int role) const=0;
	virtual QVariant nodeData(const QModelIndex& index,int role) const=0;

	VModelData* data_;
	IconFilter *icons_;
	bool active_;
};

#endif
