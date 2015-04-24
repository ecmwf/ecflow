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

class Node;
class ServerHandler;
class VAttribute;
class VServer;

//Describes the major changes during an update
class VNodeChange
{
public:
	VNodeChange() : cachedAttrNum_(-1), attrNum_(-1), cachedNodeNum_(-1),
					nodeNum_(-1), nodeAddedAt_(-1), ignore_(false) {}
	int cachedAttrNum_;
	int attrNum_;
	int cachedNodeNum_;
	int nodeNum_;
	int nodeAddedAt_;
	bool ignore_;
};

//Describes the major changes during an update
class VServerChange
{
public:
	VServerChange() : suiteNum_(-1) {}
	int suiteNum_;
};

class VNode
{
friend class VServer;

public:
	VNode(VNode* parent,Node*);
	virtual ~VNode() {};

	virtual ServerHandler* server() const;
    Node *node() {return node_;}
    bool isTopLevel() const;

    void beginUpdateAttrNum();
    void endUpdateAttrNum();
    short cachedAttrNum() const;
    short attrNum() const;

    QStringList getAttributeData(int,VAttribute**);

    VNode* parent() const {return parent_;}
    int numOfChildren() const { return static_cast<int>(children_.size());}
    VNode* childAt(int index) const;
    int indexOfChild(const VNode* vn) const;
    int indexOfChild(Node* n) const;

    std::string genVariable(const std::string& key) const;

    virtual std::string findVariable(const std::string& key,bool substitute=false) const;

    //Find a variable in the given node or in its ancestors. Both the variables and the
    //generated variables are searched.
    virtual std::string findInheritedVariable(const std::string& key,bool substitute=false) const;

    virtual std::string absNodePath() const;
    QString name() const;
    virtual  QString stateName();
    virtual QString defaultStateName();
    virtual bool isSuspended() const;
    virtual QColor  stateColour() const;

    LogServer_ptr logServer();

protected:
    void replaceChildren(const std::vector<VNode*>& newCh);
    void addChild(VNode*);
    short currentAttrNum() const;
    bool isAttrNumInitialised() const {return attrNum_!=-1;}

    Node* node_;
    VNode* parent_;
    std::vector<VNode*> children_;
    mutable short attrNum_;
    mutable short cachedAttrNum_;
};

//This is the root node representing the Server.

class VServer : public VNode
{
	friend class ServerHandler;

public:
	VServer(ServerHandler*);
	~VServer();

	ServerHandler* server() const {return server_;}

	int totalNum() const {return totalNum_;}
	VNode* toVNode(const Node* nc) const;
	void beginUpdate(VNode* node,const std::vector<ecf::Aspect::Type>& aspect,VNodeChange&);
	void endUpdate(VNode* node,const std::vector<ecf::Aspect::Type>& aspect);

	QString toolTip();

	//From VNode
	std::string absNodePath() const {return "/";}
	QString stateName();
	QString defaultStateName();
	bool isSuspended() const;
	QColor  stateColour() const;

	//Find a variable in the Defs. Both the user_variables and the
	//server variables are searched.
	std::string findVariable(const std::string& key,bool substitute=false) const;
	std::string findInheritedVariable(const std::string& key,bool substitute=false) const;

protected:
	//Clear contents and rebuild the whole tree.
	void beginScan(VServerChange&);
	void endScan();

private:
	void clear();
    void scan(VNode*);
    void deleteNode(VNode* node);

    ServerHandler* server_;
    int totalNum_;
};




#endif
