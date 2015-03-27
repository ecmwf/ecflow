//============================================================================
// Copyright 2015 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef VMODELDATA_H
#define VMODELDATA_H

#include <vector>

#include <QObject>

#include "NodeObserver.hpp"
#include "ServerFilter.hpp"

class Node;
class VNode;

class NodeFilter;
class NodeFilterDef;
class ServerHandler;
class VParamSet;

class VModelServer : public QObject, public NodeObserver
{
Q_OBJECT

friend class VModelData;

public:
    VModelServer(ServerHandler *server);
    virtual ~VModelServer();

    ServerHandler* realServer() const {return server_;}
    int topLevelNodeNum() const;
    int indexOfTopLevelNode(VNode* node) const;
    VNode* topLevelNode(int row) const;
    int totalNodeNum() const;
    void runFilter();

    //From NodeObserver
	void notifyNodeChanged(const Node*, const std::vector<ecf::Aspect::Type>&) {};

Q_SIGNALS:
	void addRemoveAttributes(VModelServer*,VNode*,int,int);
	void dataChanged(VModelServer*);
	void dataChanged(VModelServer*,VNode*);

protected:
	ServerHandler *server_;
	NodeFilter* filter_;
};

class VTreeServer : public VModelServer
{
public:
	 VTreeServer(ServerHandler *server,NodeFilterDef* filterDef);
	 ~VTreeServer();

	 int checkAttributeUpdateDiff(VNode *node);

	 //From NodeObserver
	 void notifyNodeChanged(VNode*, const std::vector<ecf::Aspect::Type>&);
};

class VTableServer : public VModelServer
{
public:
	 VTableServer(ServerHandler *server,NodeFilterDef* filterDef);
	 ~VTableServer();

	 //From NodeObserver
	 void notifyNodeChanged(VNode*, const std::vector<ecf::Aspect::Type>&);
};

//This class defines the data a given Node Model (Tree or Table) displays. The
//node models can only access data through this interface. The class internally stores
//a vector of VModelServer objects each representing a server the model displays.

class VModelData : public QObject, public ServerFilterObserver
{
Q_OBJECT

public:
	enum ModelType {TreeModel,TableModel};

	VModelData(ServerFilter*,NodeFilterDef* filterDef,ModelType);
	~VModelData();

	void clear();
	void reset(ServerFilter* servers);
	void runFilter(bool broadcast);

	int  indexOfServer(void*) const;
	bool identifyTopLevelNode(VNode* node,VModelServer**,int& index);
	VNode* topLevelNode(void*,int);

	ServerHandler* realServer(int) const;
	VModelServer* server(int) const;
	int indexOfServer(ServerHandler* s) const;

	int count() const {return static_cast<int>(servers_.size());}
	int numOfNodes(int) const;
	int numOfFiltered(int index) const;
	bool isFiltered(VNode *node) const;
	VNode* getNodeFromFilter(int totalRow);

	//From ServerFilterObserver
	void notifyServerFilterAdded(ServerItem*);
	void notifyServerFilterRemoved(ServerItem*);
	void notifyServerFilterChanged(ServerItem*);

public Q_SLOTS:
	void slotFilterDefChanged();

Q_SIGNALS:
	void filterChanged();
	void serverAddBegin(int);
	void serverAddEnd();
	void serverRemoveBegin(int);
	void serverRemoveEnd();
	void addRemoveAttributes(VModelServer*,VNode*,int,int);
	void dataChanged(VModelServer*);
	void dataChanged(VModelServer*,VNode*);

protected:
	void init();
	void add(ServerHandler*);

	ModelType modelType_;
	std::vector<VModelServer*> servers_;
	ServerFilter *serverFilter_;
	NodeFilterDef* filterDef_;
};

#endif
