//============================================================================
// Copyright 2014 ECMWF.
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

#include <QColor>
#include <QStringList>

#include "Aspect.hpp"
#include "LogServer.hpp"
#include "Node.hpp"

class IconFilter;
class ServerHandler;
class VAttribute;
class VServer;
class VServerSettings;

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
	VServerChange() : suiteNum_(0), attrNum_(0), totalNum_(0) {}
	int suiteNum_;
	int attrNum_;
	int totalNum_;
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

class VNode
{
friend class VServer;

public:
	VNode(VNode* parent,node_ptr);
	virtual ~VNode() {};

	enum SortMode {ParentToChildSort,ChildToParentSort};

	virtual ServerHandler* server() const;
    node_ptr node() const {return node_;}
    virtual bool isTopLevel() const;
    virtual bool isServer() const {return false;}

    void beginUpdateAttrNum();
    void endUpdateAttrNum();
    short cachedAttrNum() const;
    short attrNum() const;

    QStringList getAttributeData(int,VAttribute*&);
    bool getAttributeData(const std::string& type,int row, QStringList&);
    VAttribute* getAttributeType(int);
    int getAttributeLineNum(int row);

    VNode* parent() const {return parent_;}
    int numOfChildren() const { return static_cast<int>(children_.size());}
    VNode* childAt(int index) const;
    int indexOfChild(const VNode* vn) const;
    int indexOfChild(node_ptr n) const;
    VNode* findChild(const std::string& name) const;
    void collect(std::vector<VNode*>& vec) const;

    //Get all the variables
    virtual int variablesNum() const;
    virtual int genVariablesNum() const;
    virtual void variables(std::vector<Variable>& vars);
    virtual void genVariables(std::vector<Variable>& genVars);

    virtual std::string genVariable(const std::string& key) const;
    virtual std::string findVariable(const std::string& key,bool substitute=false) const;

    //Find a variable in the given node or in its ancestors. Both the variables and the
    //generated variables are searched.
    virtual std::string findInheritedVariable(const std::string& key,bool substitute=false) const;

    virtual std::string absNodePath() const;
    bool sameName(const std::string& name) const;
    virtual std::string strName() const;
    virtual QString name() const;
    std::string serverName() const;
    virtual QString stateName();
    virtual QString serverStateName();
    virtual QString defaultStateName();
    virtual bool isDefaultStateComplete();
    virtual bool isSuspended() const;
    virtual QColor  stateColour() const;
    virtual QColor  realStateColour() const;
    virtual QColor  stateFontColour() const;
    virtual int tryNo() const;
    virtual void internalState(VNodeInternalState&) {};

    bool hasAccessed() const;
    bool isAncestor(const VNode* n);
    std::vector<VNode*> ancestors(SortMode sortMode);
    VNode* ancestorAt(int idx,SortMode sortMode);

    virtual bool isFlagSet(ecf::Flag::Type f) const;

    int index() const {return index_;}

    const std::string& nodeType();
    virtual QString toolTip();
    
    virtual void why(std::vector<std::string>& theReasonWhy) const;
    const std::string&  abortedReason() const;

    LogServer_ptr logServer();
    bool logServer(std::string& host,std::string& port);


protected:
    void clear();
    void addChild(VNode*);
    void removeChild(VNode*);
    short currentAttrNum() const;
    bool isAttrNumInitialised() const {return attrNum_!=-1;}
    VNode* find(const std::vector<std::string>& pathVec);
    virtual void check(VServerSettings* conf,bool) {};
    virtual void check(VServerSettings* conf,const VNodeInternalState&) {};
    void setIndex(int i) {index_=i;}

    //Node* node_;
    node_ptr node_;
    VNode* parent_;
    std::vector<VNode*> children_;
    mutable short attrNum_;
    mutable short cachedAttrNum_;
    int index_;
};

//This is the root node representing the Server.
class VServer : public VNode
{
	friend class ServerHandler;

public:
	explicit VServer(ServerHandler*);
	~VServer();

	ServerHandler* server() const {return server_;}

	bool isEmpty() const { return numOfChildren() == 0;}
	bool isTopLevel() const {return false;}
	bool isServer() const {return true;}

	int totalNum() const {return totalNum_;}
	int totalNumOfTopLevel(int) const;
	int totalNumOfTopLevel(VNode*) const;

	VNode* toVNode(const Node* nc) const;
	void beginUpdate(VNode* node,const std::vector<ecf::Aspect::Type>& aspect,VNodeChange&);
	void endUpdate(VNode* node,const std::vector<ecf::Aspect::Type>& aspect,const VNodeChange&);
	void beginUpdate(const std::vector<ecf::Aspect::Type>& aspect);

	VNode* nodeAt(int) const;
	const std::vector<VNode*>& nodes() const {return nodes_;}

	QString toolTip();

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
	void variables(std::vector<Variable>& vars);
	void genVariables(std::vector<Variable>& genVars);
	std::string genVariable(const std::string& key) const;

	//Find a variable in the Defs. Both the user_variables and the
	//server variables are searched.
	std::string findVariable(const std::string& key,bool substitute=false) const;
	std::string findInheritedVariable(const std::string& key,bool substitute=false) const;

	bool isFlagSet(ecf::Flag::Type f) const;

	void why(std::vector<std::string>& theReasonWhy) const;

protected:
	//Clear contents and rebuild the whole tree.
	void beginScan(VServerChange&);
	void endScan();

private:
	void clear();
	//void clear(VNode*);
    void scan(VNode*,bool);
    void deleteNode(VNode* node,bool);
    std::string substituteVariableValue(const std::string& val) const;
    void updateCache();
    void updateCache(defs_ptr defs);

    ServerHandler* server_;
    int totalNum_;
    std::vector<int> totalNumInChild_;
    std::vector<VNode*> nodes_;

    VServerCache cache_;
    std::vector<Variable> prevGenVars_;
    ecf::Flag prevFlag_;

    std::map<std::string,VNodeInternalState> prevNodeState_;
};




#endif
