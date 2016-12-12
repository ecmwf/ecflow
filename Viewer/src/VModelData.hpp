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
#include "VInfo.hpp"

class Node;
class VNode;
class VServer;

class AbstractNodeModel;
class AttributeFilter;
class NodeFilter;
class NodeFilterDef;
class NodeQUery;
class ServerHandler;
class TableNodeFilter;
class TreeNodeFilter;
class VParamSet;
class VAttribute;
class VAttributeType;
class VTreeServer;
class VTableServer;
class VTree;
class VTreeNode;


class VTreeChangeInfo
{
public:
    VTreeChangeInfo() {}
    void addStateChange(const VNode*);
    const std::vector<VNode*> stateChangeSuites() const { return stateSuites_;}
    void clear() {stateSuites_.clear();}

protected:
    std::vector<VNode*> stateSuites_;
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
    virtual int nodeNum() const = 0;
    virtual void reload()=0;
    //virtual void filterChanged()=0;
    bool inScan() const {return inScan_;}
    virtual NodeFilter* filter() const=0;

    //Performance hack to avoid casts
    virtual VTreeServer* treeServer() const {return NULL;}
    virtual VTableServer* tableServer() const {return NULL;}

Q_SIGNALS:
    void dataChanged(VModelServer*);
	void beginServerScan(VModelServer*,int);
	void endServerScan(VModelServer*,int);
	void beginServerClear(VModelServer*,int);
	void endServerClear(VModelServer*,int);

	void rerender();

protected:
	ServerHandler *server_;
    bool inScan_;
};

class VTreeServer : public VModelServer
{
Q_OBJECT

public:
     VTreeServer(ServerHandler *server,NodeFilterDef* filterDef,AttributeFilter *attrFilter);
	 ~VTreeServer();

     void reload();
     void filterChanged();
     int nodeNum() const;
     NodeFilter* filter() const;
     VTree* tree() const {return tree_;}
     VTreeServer* treeServer() const {return const_cast<VTreeServer*>(this);}
     void attrFilterChanged();
     void setForceShowNode(const VNode* node);
     void setForceShowAttribute(const VAttribute* node);
     void clearForceShow(const VItem*);

    //From ServerObserver
	 void notifyDefsChanged(ServerHandler* server, const std::vector<ecf::Aspect::Type>& a);
     void notifyServerDelete(ServerHandler*);
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
     void beginFilterUpdateRemove(VTreeServer*,const VTreeNode*,int);
     void endFilterUpdateRemove(VTreeServer*,const VTreeNode*,int);
     void beginFilterUpdateAdd(VTreeServer*,const VTreeNode*,int);
     void endFilterUpdateAdd(VTreeServer*,const VTreeNode*,int);
     void beginFilterUpdateRemoveTop(VTreeServer*,int);
     void endFilterUpdateRemoveTop(VTreeServer*,int);
     void beginFilterUpdateInsertTop(VTreeServer*,int);
     void endFilterUpdateInsertTop(VTreeServer*,int);

private:
     void updateFilter(const std::vector<VNode*>& suitesChanged);

     VTree* tree_;
     VTreeChangeInfo* changeInfo_;
     AttributeFilter *attrFilter_;
     TreeNodeFilter* filter_;
};

class VTableServer : public VModelServer
{
 Q_OBJECT
public:
	 VTableServer(ServerHandler *server,NodeFilterDef* filterDef);
	 ~VTableServer();

     void reload();
     void filterChanged() {}
     int nodeNum() const;
     VNode* nodeAt(int) const;
     int indexOf(const VNode* node) const;
     NodeFilter* filter() const;
     VTableServer* tableServer() const {return const_cast<VTableServer*>(this);}

	 //From ServerObserver
     void notifyDefsChanged(ServerHandler* server, const std::vector<ecf::Aspect::Type>& a) {}
     void notifyServerDelete(ServerHandler*);
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
     void nodeChanged(VTableServer*,const VNode*);

private:
     TableNodeFilter* filter_;
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

    void setActive(bool);
    void clear();
	void reset(ServerFilter* servers);

    void reload(bool broadcast);

	int count() const {return static_cast<int>(servers_.size());}
	int  indexOfServer(void*) const;
    ServerHandler* serverHandler(void*) const;
    ServerHandler* serverHandler(int) const;
	VModelServer* server(int) const;
    VModelServer* server(const void*) const;
    VModelServer* server(const std::string&) const;
    VModelServer* server(ServerHandler*) const;
	int indexOfServer(ServerHandler* s) const;
	int numOfNodes(int) const;
    bool isFilterComplete() const;
    bool isFilterNull() const;

	//From ServerFilterObserver
	void notifyServerFilterAdded(ServerItem*);
	void notifyServerFilterRemoved(ServerItem*);
	void notifyServerFilterChanged(ServerItem*);
	void notifyServerFilterDelete();

public Q_SLOTS:
    void slotFilterDefChanged();

Q_SIGNALS:
    //void filterChanged();
	void filterDeleteBegin();
	void filterDeleteEnd();
    void filterChangeBegun();
    void filterChangeEnded();
    void serverAddBegin(int);
	void serverAddEnd();
    void serverRemoveBegin(VModelServer*,int);
    void serverRemoveEnd(int);

protected:
	void init();
	virtual void add(ServerHandler*)=0;
    virtual void connectToModel(VModelServer* d);

	std::vector<VModelServer*> servers_;
	ServerFilter *serverFilter_;
	NodeFilterDef* filterDef_;
	AbstractNodeModel* model_;
    bool active_;
};

class VTreeModelData : public VModelData
{
    Q_OBJECT

public:
    VTreeModelData(NodeFilterDef* filterDef,AttributeFilter* attrFilter,AbstractNodeModel* model);

protected Q_SLOTS:
    void slotAttrFilterChanged();

protected:
    void add(ServerHandler *server);
    void connectToModel(VModelServer* d);

    AttributeFilter *attrFilter_;
};

class VTableModelData : public VModelData
{
public:
	VTableModelData(NodeFilterDef* filterDef,AbstractNodeModel* model);

	int numOfFiltered(int index) const;
    VNode* nodeAt(int totalRow);
    int position(const VNode *node) const;
    int position(VTableServer*,const VNode *node) const;
    int position(VTableServer* server);
    bool position(VTableServer* server,int& start,int& count);

protected:
    void add(ServerHandler *server);
    void connectToModel(VModelServer* d);
};

#endif
