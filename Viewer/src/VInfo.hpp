//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef VINFO_HPP_
#define VINFO_HPP_

#include <cstddef>
#include <boost/shared_ptr.hpp>

#include <string>
#include <vector>

#include "Variable.hpp"

class ServerHandler;
class VAttribute;
class VNode;

class VInfoVisitor;

//==============================================================================
// For each selected item in any of the views a new VInfo object is created.
// This class offers the same interface to access information about any kind of
// selected items: servers, nodes, attributes. The concrete implementation of
// these access methods are done in subclasses derived from VInfo.
//
// VInfo is regarded as a temporary object. We only need it while the selection
// is used in breadcrumbs, info panels or other widgets outside the main views.
//==============================================================================

class VInfo
{
public:
	enum NodeOrder {ChildToParentOrder,ParentToChildOrder};

	VInfo();
	virtual ~VInfo() {};

	virtual bool isServer() {return false;}
	virtual bool isNode()  {return false;}
	virtual bool isAtrribute()  {return false;}
	virtual bool isEmpty()  {return true;}

	virtual ServerHandler* server() {return server_;};
	virtual VNode* node()  {return node_;}
	virtual VAttribute* attribute() {return att_;}

	virtual void variables(std::vector<Variable>& vars) {};
	virtual void genVariables(std::vector<Variable>& vars) {};
	virtual std::string name() {return std::string();}
	virtual std::string fullPath() {return std::string();}

	void ancestors(ServerHandler **server,std::vector<VNode*>& nodes);
	std::vector<VNode*> ancestors(NodeOrder);
    bool sameAs(const VNode* n,bool checkAncestors=false);
	
    virtual void accept(VInfoVisitor*)=0;

    static const std::string&  nodeType(VNode*);

	static VInfo* make(ServerHandler*);
	static VInfo* make(VNode*,ServerHandler* server=0);
	static VInfo* make(VAttribute*,int,VNode*);

protected:
	VInfo(ServerHandler*);
	VInfo(VNode*,ServerHandler* server);
	VInfo(VAttribute*,int attIndex,VNode*,ServerHandler* server);

	mutable ServerHandler* server_;
	mutable VNode* node_;
	mutable VAttribute* att_;
	mutable int attIndex_;
};



typedef boost::shared_ptr<VInfo>   VInfo_ptr;


// Implements the info object for server selections
class VInfoServer : public VInfo
{
public:
	VInfoServer(ServerHandler*);

	bool isServer() {return true;}
    bool isEmpty() {return false;}
    void accept(VInfoVisitor*);

    void variables(std::vector<Variable>& vars);
    void genVariables(std::vector<Variable>& vars);
    std::string name();
    std::string fullPath() {return name();}
};

// Implements the info object for node selections
class VInfoNode: public VInfo
{
public:
	VInfoNode(VNode*,ServerHandler* server=0);

	bool isNode() {return true;}
	bool isEmpty() {return false;}

	ServerHandler* server();
	void accept(VInfoVisitor*);
	const std::string&  nodeType();

	void variables(std::vector<Variable>& vars);
	void genVariables(std::vector<Variable>& vars);
	std::string name();
	std::string fullPath();


protected:
};

// Implements the info  base class for attribute selections
class VInfoAttribute: public VInfo
{
public:
	VInfoAttribute(VAttribute*,int,VNode*,ServerHandler*);

	bool isAttribute() {return true;}
	bool isEmpty() {return false;}
	void accept(VInfoVisitor*);
};

// Implements the info object limit attributes
class VInfoLimit: public VInfoAttribute
{
public:
	VInfoLimit(VAttribute*,int,VNode*,ServerHandler* server=0);
};

//=================================================
// Factory to make attribute info objects
//=================================================

class VInfoAttributeFactory
{
public:
	VInfoAttributeFactory(const std::string&);
	virtual ~VInfoAttributeFactory();

	virtual VInfoAttribute* make(VAttribute*,int,VNode*,ServerHandler* server=0) = 0;
	static VInfoAttribute* create(VAttribute* att,int attIndex,VNode* node,ServerHandler* server=0);

private:
	VInfoAttributeFactory(const VInfoAttributeFactory&);
	VInfoAttributeFactory& operator=(const VInfoAttributeFactory &);

};

template<class T>
class  VInfoAttributeMaker : public VInfoAttributeFactory
{
	VInfoAttribute* make(VAttribute* att,int attIndex,VNode* node,ServerHandler* server=0)
	       { return new T(att,attIndex,node,server); }
public:
	 VInfoAttributeMaker(const std::string& name) : VInfoAttributeFactory(name) {}
};

typedef boost::shared_ptr<VInfoServer>   VInfoServer_ptr;
typedef boost::shared_ptr<VInfoNode>   VInfoNode_ptr;
typedef boost::shared_ptr<VInfoAttribute>   VInfoAttribute_ptr;


class VInfoVisitor
{
public:
	VInfoVisitor() {};
	virtual ~VInfoVisitor() {};

	virtual void visit(VInfoServer*)=0;
	virtual void visit(VInfoNode*)=0;
	virtual void visit(VInfoAttribute*)=0;

};



/*class VInfoVisitor
{
public:
	VInfoVisitor() {};
	virtual ~VInfoVisitor() {};

	virtual void visit(boost::shared_ptr<VInfoServer>)=0;
	virtual void visit(boost::shared_ptr<VInfoNode>)=0;
	virtual void visit(boost::shared_ptr<VInfoAttribute>)=0;
};
*/


/*
#include "ViewNodeInfo.hpp"

#include "ServerHandler.hpp"

ViewNodeInfo::ViewNodeInfo() : node_(NULL),server_(NULL)
{
}

ViewNodeInfo::ViewNodeInfo(Node *node,ServerHandler* server) : node_(node), server_(server)
{
}

ViewNodeInfo::ViewNodeInfo(ServerHandler* server) : node_(NULL),server_(server)
{

}

bool ViewNodeInfo::isNode() const
{
	return (node_ != NULL);
}

bool ViewNodeInfo::isServer() const
{
	return (node_ == NULL && server_ != NULL);
}

bool ViewNodeInfo::isEmpty() const
{
	return (node_ == NULL && server_ == NULL);
}

Node* ViewNodeInfo::node() const
{
	return node_;
}

ServerHandler* ViewNodeInfo::server() const
{
	if(server_ == NULL && node_)
	{
		server_=ServerHandler::find(node_);
	}
	return server_;
}
*/



#endif

