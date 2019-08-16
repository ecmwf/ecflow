//============================================================================
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef VNODE_HPP_
#define VNODE_HPP_

#include <vector>
#include <set>

#include <QColor>
#include <QStringList>

#include "Aspect.hpp"
#include "Node.hpp"

#include "VItem.hpp"

class AttributeFilter;
class IconFilter;
class ServerHandler;
class TriggerCollector;
class TriggeredScanner;
class VAttributeType;
class VServer;
class VServerSettings;
class VNodeTriggerData;

class VNodeInternalState
{
public:
	VNodeInternalState() : tryNo_(0), flag_(0) {}

	unsigned char tryNo_;
	unsigned char flag_;
};


//Describes the major changes during an update
class VNodeChange
{
public:
	VNodeChange() : cachedAttrNum_(-1), attrNum_(-1), cachedNodeNum_(-1),
					nodeNum_(-1), nodeAddAt_(-1), nodeRemoveAt_(-1),
					ignore_(false), rescan_(false) {}
	int cachedAttrNum_;
	int attrNum_;
	int cachedNodeNum_;
	int nodeNum_;
	int nodeAddAt_;
	int nodeRemoveAt_;
	bool ignore_;
	bool rescan_;
};

//Describes the major changes during an update
class VServerChange
{
public:
    VServerChange() : suiteNum_(0), attrNum_(0) {} //, totalNum_(0) {}
	int suiteNum_;
	int attrNum_;
    //int totalNum_;
};

class VServerCache
{
public:
	std::vector<Variable> vars_;
    std::vector<Variable> genVars_;
    ecf::Flag flag_;

    void clear() {
    	vars_.clear();
    	genVars_.clear();
    	flag_.reset();
    }
};

class VNode : public VItem
{
friend class VServer;
friend class VLabelAttr;
friend class VMeterAttr;
friend class VEventAttr;
friend class VRepeatAttr;
friend class VTriggerAttr;
friend class VLimitAttr;
friend class VLimiterAttr;
friend class VLateAttr;
friend class VTimeAttr;
friend class VDateAttr;
friend class VGenVarAttr;
friend class VUserVarAttr;

public:
	VNode(VNode* parent,node_ptr);
    virtual ~VNode();

	enum SortMode {ParentToChildSort,ChildToParentSort};

    VServer *root() const;
    ServerHandler* server() const;
    virtual VNode* suite() const;
    node_ptr node() const {return node_;}  
    VNode* isNode() const {return const_cast<VNode*>(this);}
    bool isTopLevel() const;

    //Attributes
    const std::vector<VAttribute*>& attr() const {return attr_;}
    int attrNum(AttributeFilter* filter=0) const;
    VAttribute* attribute(int,AttributeFilter *filter=0) const;
    VAttribute* attributeForType(int,VAttributeType*) const;
    int indexOfAttribute(const VAttribute* a, AttributeFilter *filter) const;
    VAttribute* findAttribute(QStringList aData);
    VAttribute* findAttribute(const std::string& typeName,const std::string& name);
    VAttribute* findAttribute(VAttributeType* type,const std::string& name);

    int numOfChildren() const { return static_cast<int>(children_.size());}
    VNode* childAt(int index) const;
    int indexOfChild(const VNode* vn) const;
    int indexOfChild(node_ptr n) const;
    VNode* findChild(const std::string& name) const;
    void collect(std::vector<VNode*>& vec) const;
    void collectAbortedTasks(std::vector<VNode*>& vec) const;

    //Get all the variables
    virtual int variablesNum() const;
    virtual int genVariablesNum() const;
    virtual void variables(std::vector<Variable>& vars) const;
    virtual void genVariables(std::vector<Variable>& genVars) const;

    virtual std::string genVariable(const std::string& key) const;
    virtual std::string findVariable(const std::string& key,bool substitute=false) const;
    virtual bool substituteVariableValue(std::string& val) const;

    virtual void collectInheritedVariableNames(std::set<std::string>& vars) const;

    //Find a variable in the given node or in its ancestors. Both the variables and the
    //generated variables are searched.
    virtual std::string findInheritedVariable(const std::string& key,bool substitute=false) const;

    std::string fullPath() const;
    virtual std::string absNodePath() const;

    bool sameName(const std::string& name) const;
    bool sameContents(VItem* item) const;
    std::string strName() const;
    QString name() const;
    std::string serverName() const;
    virtual QString stateName();
    virtual QString serverStateName();
    virtual QString defaultStateName();
    virtual bool isDefaultStateComplete();
    virtual bool isSuspended() const;
    virtual bool isAborted() const;
    virtual bool isSubmitted() const;
    virtual QColor  stateColour() const;
    virtual QColor  realStateColour() const;
    virtual QColor  stateFontColour() const;
    virtual QColor  typeFontColour() const;
    virtual int tryNo() const;
    virtual void internalState(VNodeInternalState&) {}

    bool hasAccessed() const;
    std::vector<VNode*> ancestors(SortMode sortMode);
    VNode* ancestorAt(int idx,SortMode sortMode);


    virtual std::string flagsAsStr() const;
    virtual bool isFlagSet(ecf::Flag::Type f) const;

    int index() const {return index_;}

    const std::string& nodeType();
    virtual QString toolTip();
    
    virtual void why(std::vector<std::string>& bottomUp,
                     std::vector<std::string>& topDown) const;

    virtual void why(std::vector<std::string>& theReasonWhy) const {}

    const std::string&  abortedReason() const;  
    void statusChangeTime(QString&) const;
    unsigned int statusChangeTime() const;

    bool logServer(std::string& host,std::string& port);

    void triggerExpr(std::string&,std::string&) const;

    void triggers(TriggerCollector*);   
    void triggered(TriggerCollector* tlc,TriggeredScanner* scanner=0);
    void clearTriggerData();
    void addTriggeredData(VItem* n);
    void addTriggeredData(VItem* a,VAttribute* n);
    void triggeredByEvent(const std::string& name,std::vector<std::string>& triggeredVec,TriggeredScanner* scanner);

    QString nodeMenuMode() const;
    QString defStatusNodeMenuMode() const;

    virtual void print();

protected:
    void clear();
    void addChild(VNode*);
    void removeChild(VNode*);
    void scanAttr();
    void rescanAttr();
    void findAttributes(VAttributeType*,std::vector<VAttribute*>& v);

    VNode* find(const std::vector<std::string>& pathVec);
    virtual void check(VServerSettings* conf,bool) {}
    virtual void check(VServerSettings* conf,const VNodeInternalState&) {}
    void setIndex(int i) {index_=i;}

    VAttribute* findLimit(const std::string& path, const std::string& name);
    static void triggersInChildren(VNode *n,VNode* nn,TriggerCollector* tlc);
    static void triggeredByChildren(VNode *n,VNode* parent,TriggerCollector* tlc);

    node_ptr node_;
    std::vector<VNode*> children_;
    std::vector<VAttribute*> attr_;
    int index_;
    VNodeTriggerData* data_;
};

class VSuiteNode : public VNode
{
public:
    VSuiteNode(VNode* parent,node_ptr node) : VNode(parent,node) {}
    VSuiteNode* isSuite() const {return const_cast<VSuiteNode*>(this);}
    const std::string& typeName() const;
};

class VFamilyNode : public VNode
{
public:
    VFamilyNode(VNode* parent,node_ptr node) : VNode(parent,node) {}
    VFamilyNode* isFamily() const {return const_cast<VFamilyNode*>(this);}
    const std::string& typeName() const;
};

class VAliasNode : public VNode
{
public:
    VAliasNode(VNode* parent,node_ptr node) : VNode(parent,node) {}
    VAliasNode* isAlias() const {return const_cast<VAliasNode*>(this);}
    const std::string& typeName() const;
};

//This is the root node representing the Server.
class VServer : public VNode
{
	friend class ServerHandler;
    friend class VNode;

public:
	explicit VServer(ServerHandler*);
	~VServer();

	ServerHandler* server() const {return server_;}
    VNode* suite() const {return NULL;}

	bool isEmpty() const { return numOfChildren() == 0;}
	bool isTopLevel() const {return false;}
    VServer* isServer() const {return const_cast<VServer*>(this);}
    VNode* isNode() const {return NULL;}

	int totalNum() const {return totalNum_;}
	int totalNumOfTopLevel(int) const;
    int totalNumOfTopLevel(VNode*) const;
    int totalNumOfTopLevel(const std::string&) const;

	VNode* toVNode(const Node* nc) const;
	void beginUpdate(VNode* node,const std::vector<ecf::Aspect::Type>& aspect,VNodeChange&);
	void endUpdate(VNode* node,const std::vector<ecf::Aspect::Type>& aspect,const VNodeChange&);
	void beginUpdate(const std::vector<ecf::Aspect::Type>& aspect);

	VNode* nodeAt(int) const;
	const std::vector<VNode*>& nodes() const {return nodes_;}

    const std::string& typeName() const override;
    QString toolTip() override;

	//From VNode
	std::string absNodePath() const {return "/";}
	QString stateName();
	QString defaultStateName();
    QString serverStateName();
	bool isSuspended() const;
	QColor  stateColour() const;
	QColor  stateFontColour() const;
	std::string strName() const;
	int tryNo() const {return 0;}

	void suites(std::vector<std::string>&);
	VNode* find(const std::string& fullPath);

	//Get all the variables
    int variablesNum() const;
	int genVariablesNum() const;
    void variables(std::vector<Variable>& vars) const;
    void genVariables(std::vector<Variable>& genVars) const;
	std::string genVariable(const std::string& key) const;

	//Find a variable in the Defs. Both the user_variables and the
	//server variables are searched.
	std::string findVariable(const std::string& key,bool substitute=false) const;
	std::string findInheritedVariable(const std::string& key,bool substitute=false) const;
    bool substituteVariableValue(std::string& val) const;

    std::string flagsAsStr() const;
	bool isFlagSet(ecf::Flag::Type f) const;

	void why(std::vector<std::string>& theReasonWhy) const;

    bool triggeredScanned() const {return triggeredScanned_;}

    void print();

protected:
	//Clear contents and rebuild the whole tree.
	void beginScan(VServerChange&);
	void endScan();
    void setTriggeredScanned(bool b) {triggeredScanned_=b;}
    void clearNodeTriggerData();

private:
	void clear();
	//void clear(VNode*);
    void scan(VNode*,bool);
    void deleteNode(VNode* node,bool);
    void updateCache();
    void updateCache(defs_ptr defs);

    ServerHandler* server_;
    int totalNum_;   
    std::vector<int> totalNumInChild_;
    std::vector<VNode*> nodes_;
    bool triggeredScanned_;

    VServerCache cache_;
    std::vector<Variable> prevGenVars_;
    ecf::Flag prevFlag_;

    std::map<std::string,VNodeInternalState> prevNodeState_;
};




#endif
