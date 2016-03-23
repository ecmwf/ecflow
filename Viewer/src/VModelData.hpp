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
#include "ServerObserver.hpp"

class Node;
class VNode;
class VServer;

class AbstractNodeModel;
class NodeFilter;
class NodeFilterDef;
class NodeQUery;
class ServerHandler;
class VParamSet;

class VTreeServer;
class VTableServer;

class VTreeChangeInfo
{
public:
    VTreeChangeInfo() {}
    void addStateChange(const VNode*);
    QList<VNode*> stateChangeSuites() const { return stateSuites_;}
    void clear() {stateSuites_.clear();}

protected:
    QList<VNode*> stateSuites_;
};


class VTreeNode
{
public:
    VTreeNode(VNode* vnode,VTreeNode* parent);

    VNode* vnode() const {return vnode_;}
    void addChild(VTreeNode*);
    int numOfChildren() const {return children_.size();}
    int indexOfChild(const VTreeNode* vn) const;
    int indexInParent() const;
    VTreeNode* childAt(int i) {return children_[i];}
    VTreeNode* parent() const {return parent_;}

    int attrNum() const {return attrNum_;}
    bool isTopLevel() const {if(parent_) return (parent_->parent())?false:true; return false;}

public:
    VNode* vnode_;
    std::vector<VTreeNode*> children_;
    VTreeNode *parent_;
    int attrNum_;
};


class VModelServer : public QObject, public ServerObserver, public NodeObserver
{
Q_OBJECT

friend class VModelData;
friend class VTableModelData;
friend class VTreeModelData;

public:
    explicit VModelServer(ServerHandler *server);
    virtual ~VModelServer();

    ServerHandler* realServer() const {return server_;}

    int totalNodeNum() const;
    virtual void runFilter()=0;
    bool inScan() const {return inScan_;}

    //Performance hack to avoid casts
    virtual VTreeServer* treeServer() const {return NULL;}
    virtual VTableServer* tableServer() const {return NULL;}

Q_SIGNALS:
#if 0
    void beginAddRemoveAttributes(VModelServer*,const VNode*,int,int);
	void endAddRemoveAttributes(VModelServer*,const VNode*,int,int);
    void nodeChanged(VModelServer*,const VNode*);
    void attributesChanged(VModelServer*,const VNode*);
#endif

    void dataChanged(VModelServer*);
	void beginServerScan(VModelServer*,int);
	void endServerScan(VModelServer*,int);
	void beginServerClear(VModelServer*,int);
	void endServerClear(VModelServer*,int);

	void filterChanged();
	void rerender();

protected:
	ServerHandler *server_;
	NodeFilter* filter_;
	NodeFilter* cachedFilter_;
    bool inScan_;
};

class VTreeServer : public VModelServer, public VTreeNode
{
Q_OBJECT

public:
	 VTreeServer(ServerHandler *server,NodeFilterDef* filterDef);
	 ~VTreeServer();

	 int checkAttributeUpdateDiff(VNode *node);
     void runFilter();
     int topLevelNodeNum() const;
     int indexOfTopLevelNode(const VTreeNode* vnode) const;
     VTreeNode* topLevelNode(int row) const;
     VTreeNode* find(const VNode*) const;

     VTreeServer* treeServer() const {return const_cast<VTreeServer*>(this);}

     //From ServerObserver
	 void notifyDefsChanged(ServerHandler* server, const std::vector<ecf::Aspect::Type>& a);
     void notifyServerDelete(ServerHandler*) {}
	 void notifyBeginServerClear(ServerHandler* server);
	 void notifyEndServerClear(ServerHandler* server);
	 void notifyBeginServerScan(ServerHandler* server,const VServerChange&);
	 void notifyEndServerScan(ServerHandler* server);
	 void notifyServerConnectState(ServerHandler* server);
	 void notifyServerActivityChanged(ServerHandler* server);
	 void notifyEndServerSync(ServerHandler* server);

	 //From NodeObserver
	 void notifyBeginNodeChange(const VNode*, const std::vector<ecf::Aspect::Type>&,const VNodeChange&);
	 void notifyEndNodeChange(const VNode*, const std::vector<ecf::Aspect::Type>&,const VNodeChange&);

Q_SIGNALS:
     void beginAddRemoveAttributes(VTreeServer*,const VTreeNode*,int,int);
     void endAddRemoveAttributes(VTreeServer*,const VTreeNode*,int,int);
     void nodeChanged(VTreeServer*,const VTreeNode*);
     void attributesChanged(VTreeServer*,const VTreeNode*);

protected:
     void clearTree() {}
     void buildTree(const std::vector<VNode*>& filter);
     void buildTree();

private:
     void buildTree(VTreeNode* parent,VNode* vnode,const std::vector<VNode*>& filter);
     void buildTree(VTreeNode* parent,VNode* vnode);

     std::vector<VTreeNode*> nodeVec_;
     int nodeStateChangeCnt_;
     VTreeChangeInfo* changeInfo_;
};

class VTableServer : public VModelServer
{
public:
	 VTableServer(ServerHandler *server,NodeFilterDef* filterDef);
	 ~VTableServer();

     void runFilter() {}
     VTableServer* tableServer() const {return const_cast<VTableServer*>(this);}

	 //From ServerObserver
     void notifyDefsChanged(ServerHandler* server, const std::vector<ecf::Aspect::Type>& a) {}
     void notifyServerDelete(ServerHandler*) {}
	 void notifyBeginServerClear(ServerHandler* server);
     void notifyEndServerClear(ServerHandler* server);
     void notifyBeginServerScan(ServerHandler* server,const VServerChange&);
	 void notifyEndServerScan(ServerHandler* server);
	 void notifyServerConnectState(ServerHandler* server);
	 void notifyServerActivityChanged(ServerHandler* server);
	 void notifyEndServerSync(ServerHandler* server);

	 //From NodeObserver
	 void notifyBeginNodeChange(const VNode*, const std::vector<ecf::Aspect::Type>&,const VNodeChange&);
	 void notifyEndNodeChange(const VNode*, const std::vector<ecf::Aspect::Type>&,const VNodeChange&);

private:
	 bool useCachedFilter_;
	 size_t totalNumBeforeClear_;
};

//This class defines the data a given Node Model (Tree or Table) displays. The
//node models can only access data through this interface. The class internally stores
//a vector of VModelServer objects each representing a server the model displays.

class VModelData : public QObject, public ServerFilterObserver
{
Q_OBJECT

public:
	enum ModelType {TreeModel,TableModel};

	VModelData(NodeFilterDef* filterDef,AbstractNodeModel* model);
	~VModelData();

	void clear();
	void reset(ServerFilter* servers);

	void runFilter(bool broadcast);

	int count() const {return static_cast<int>(servers_.size());}
	int  indexOfServer(void*) const;
    ServerHandler* serverHandler(void*) const;
    ServerHandler* serverHandler(int) const;
	VModelServer* server(int) const;
    VModelServer* server(const void*) const;
	int indexOfServer(ServerHandler* s) const;
	int numOfNodes(int) const;
	virtual bool isFiltered(VNode *node) const=0;

	//From ServerFilterObserver
	void notifyServerFilterAdded(ServerItem*);
	void notifyServerFilterRemoved(ServerItem*);
	void notifyServerFilterChanged(ServerItem*);
	void notifyServerFilterDelete();

public Q_SLOTS:
	void slotFilterDefChanged();

Q_SIGNALS:
	void filterChanged();
	void filterDeleteBegin();
	void filterDeleteEnd();
	void serverAddBegin(int);
	void serverAddEnd();
	void serverRemoveBegin(int);
	void serverRemoveEnd();

protected:
	void init();
	virtual void add(ServerHandler*)=0;
    virtual void connectToModel(VModelServer* d);

	std::vector<VModelServer*> servers_;
	ServerFilter *serverFilter_;
	NodeFilterDef* filterDef_;
	AbstractNodeModel* model_;
};

class VTreeModelData : public VModelData
{
public:
	VTreeModelData(NodeFilterDef* filterDef,AbstractNodeModel* model);

    //bool identifyTopLevelNode(const VNode* node,VModelServer**,int& index);
    //bool identifyTopLevelNode(const VTreeNode* node,VModelServer**,int& index);
    //VNode* topLevelNode(void*,int);
	bool isFiltered(VNode *node) const;

protected:
    void add(ServerHandler *server);
    void connectToModel(VModelServer* d);

};

class VTableModelData : public VModelData
{
public:
	VTableModelData(NodeFilterDef* filterDef,AbstractNodeModel* model);

	int numOfFiltered(int index) const;
	VNode* getNodeFromFilter(int totalRow);
	int posInFilter(const VNode *node) const;
	int posInFilter(VModelServer*,const VNode *node) const;
	int pos(VModelServer* server,VNode**);
	bool identifyInFilter(VModelServer* server,int& start,int& count,VNode**);
	bool isFiltered(VNode *node) const;

protected:
	void add(ServerHandler *server);
};

#endif
