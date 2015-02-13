//============================================================================
// Copyright 2015 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef NODEMODELDATA_H
#define NODEMODELDATA_H

#include <vector>

#include <QObject>

#include "NodeObserver.hpp"
#include "ServerFilter.hpp"

class Node;

class NodeFilter;
class NodeFilterDef;
class ServerHandler;
class VParamSet;

class NodeModelData
{
friend class NodeModelDataHandler;

public:
    NodeModelData(ServerHandler *server,NodeFilter*);
    ~NodeModelData();

    int nodeNum() const;
    void runFilter();

protected:
	ServerHandler *server_;
	NodeFilter* filter_;
	mutable int nodeNum_;
};


class NodeModelDataHandler : public QObject, public ServerFilterObserver, public NodeObserver
{
Q_OBJECT

public:
	NodeModelDataHandler(NodeFilterDef* filterDef);

	void clear();
	void reset(ServerFilter* servers);
	void reload();
	void runFilter(bool broadcast);

	ServerHandler* server(int) const;
	ServerHandler* server(void*) const;
	int indexOf(ServerHandler* s) const;
	int count() const {return static_cast<int>(data_.size());}

	int numOfNodes(int) const;
	int numOfFiltered(int index) const;
	bool isFiltered(Node *node) const;
	void filter(VParamSet* stateFilter);
	Node* getNodeFromFilter(int totalRow);

	//From ServerFilterObserver
	void notifyServerFilterAdded(ServerItem*);
	void notifyServerFilterRemoved(ServerItem*);
	void notifyServerFilterChanged(ServerItem*);

	//From NodeObserver
	void notifyNodeChanged(const Node*, const std::vector<ecf::Aspect::Type>&);

public Q_SLOTS:
	void slotFilterDefChanged();

Q_SIGNALS:
	void filterChanged();
	void serverAddBegin(int);
	void serverAddEnd();
	void serverRemoveBegin(int);
	void serverRemoveEnd();

protected:
	void init();
	virtual NodeModelData* makeData(ServerHandler* s)=0;
	void add(ServerHandler*);
	NodeModelData* data(int) const;

	std::vector<NodeModelData*> data_;
	ServerFilter *servers_;
	NodeFilterDef* filterDef_;
};

class TreeNodeModelDataHandler : public  NodeModelDataHandler
{
public:
	TreeNodeModelDataHandler(NodeFilterDef* filterDef) : NodeModelDataHandler(filterDef) {};
protected:
	void init();
	NodeModelData* makeData(ServerHandler* s);
};

class TableNodeModelDataHandler : public  NodeModelDataHandler
{
public:
	TableNodeModelDataHandler(NodeFilterDef* filterDef) : NodeModelDataHandler(filterDef) {};
protected:
	NodeModelData* makeData(ServerHandler* s);
};




#endif
