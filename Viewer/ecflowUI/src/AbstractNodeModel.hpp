//============================================================================
// Copyright 2009-2019 ECMWF.
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
	explicit AbstractNodeModel(QObject *parent=0);
   	virtual ~AbstractNodeModel();

   	enum CustomItemRole {FilterRole = Qt::UserRole+1, IconRole = Qt::UserRole+2,
			     ServerRole = Qt::UserRole+3, NodeNumRole = Qt::UserRole+4,
			     InfoRole = Qt::UserRole+5, LoadRole = Qt::UserRole+6,
			     ConnectionRole = Qt::UserRole+7, ServerDataRole = Qt::UserRole+8,
			     NodeDataRole = Qt::UserRole+9, AttributeRole = Qt::UserRole+10,
			     AttributeLineRole = Qt::UserRole+11, AbortedReasonRole = Qt::UserRole + 12,
			     NodeTypeRole = Qt::UserRole + 13, NodeTypeForegroundRole = Qt::UserRole + 14,
                 ServerPointerRole = Qt::UserRole + 15, SortRole = Qt::UserRole + 16,
                 NodePointerRole = Qt::UserRole + 17, VariableRole = Qt::UserRole + 18,
                 FailedSubmissionRole = Qt::UserRole + 19};

	void dataIsAboutToChange();
	virtual VInfo_ptr nodeInfo(const QModelIndex& index)=0;
	void reload();
	void active(bool);
	bool active() const {return active_;}

	virtual VModelData* data() const = 0;
	virtual QModelIndex infoToIndex(VInfo_ptr,int column=0) const;
	virtual QModelIndex nodeToIndex(const VNode*,int column=0) const=0;
    virtual QModelIndex attributeToIndex(const VAttribute* a, int column=0) const=0;

Q_SIGNALS:
	void changed();
	void filterChanged();
	void rerender();

public Q_SLOTS:
	void slotFilterDeleteBegin();
	void slotFilterDeleteEnd();

	virtual void slotServerAddBegin(int row)=0;
	virtual void slotServerAddEnd()=0;
        virtual void slotServerRemoveBegin(VModelServer*,int)=0;
        virtual void slotServerRemoveEnd(int)=0;

	virtual void slotDataChanged(VModelServer*)=0;
	virtual void slotBeginServerScan(VModelServer* server,int)=0;
	virtual void slotEndServerScan(VModelServer* server,int)=0;
	virtual void slotBeginServerClear(VModelServer* server,int)=0;
	virtual void slotEndServerClear(VModelServer* server,int)=0;

protected:
	void init();
	void clean();
	bool hasData() const;

    virtual void resetStateFilter(bool broadcast) {}
	virtual QModelIndex serverToIndex(ServerHandler*) const=0;

	bool active_;
};

#endif
