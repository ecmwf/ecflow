//============================================================================
// Copyright 2009-2017 ECMWF.
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
#include <memory>

#include <string>
#include <vector>

#include "ServerObserver.hpp"
#include "Variable.hpp"

class ServerHandler;
class VAttribute;
class VNode;
class VItem;

class VInfoObserver;
class VInfoVisitor;

class VInfo;
typedef std::shared_ptr<VInfo> VInfo_ptr;

//==============================================================================
// For each selected item in any of the views a new VInfo object is created.
// This class offers the same interface to access information about any selected
// items: servers, nodes, attributes. The concrete implementation of
// these access methods are done in subclasses derived from VInfo.
//
// VInfo is regarded as a temporary object. We only need it while the selection
// is used in breadcrumbs, info panels or other widgets outside the main views.
//==============================================================================

class VInfo : public ServerObserver
{
public:
	virtual ~VInfo();

	virtual bool isServer() {return false;}
	virtual bool isNode()  {return false;}
    virtual bool isAttribute()  {return false;}
	virtual bool isEmpty()  {return true;}
    virtual bool hasData() const=0;

    ServerHandler* server() const {return server_;}
    VNode* node() const {return node_;}
    VAttribute* attribute() const {return attr_;}
    virtual VItem* item() const=0;

	virtual std::string name()=0;
    virtual std::string path()=0;
    virtual std::string serverAlias() {return "";}
    virtual std::string relativePath() {return "";}
    virtual std::string nodePath()=0;
    std::string storedNodePath() const;
    const std::string& storedPath() const {return storedPath_;}

    virtual void accept(VInfoVisitor*)=0;

    void regainData();
	void addObserver(VInfoObserver*);
	void removeObserver(VInfoObserver*);

	//Form ServerObserver
    void notifyDefsChanged(ServerHandler* server, const std::vector<ecf::Aspect::Type>& a) {}
    void notifyServerDelete(ServerHandler* server);
    void notifyBeginServerClear(ServerHandler* server);
    void notifyEndServerClear(ServerHandler* server);
    void notifyBeginServerScan(ServerHandler* server,const VServerChange&) {}
    void notifyEndServerScan(ServerHandler* server);
    void notifyServerConnectState(ServerHandler* server) {}
    void notifyServerActivityChanged(ServerHandler* server) {}
    void notifyServerSuiteFilterChanged(ServerHandler* server) {}

    bool operator ==(const VInfo&);

    static VInfo_ptr createParent(VInfo_ptr);
    static VInfo_ptr createFromPath(ServerHandler*,const std::string&);
    static VInfo_ptr createFromPath(const std::string& path);
    static VInfo_ptr createFromItem(VItem*);

protected:
    VInfo(ServerHandler* server,VNode* node,VAttribute* attr=nullptr);
	void dataLost();

	mutable ServerHandler* server_;
    mutable VNode* node_;
    mutable VAttribute* attr_;
    mutable std::string storedPath_;

	std::vector<VInfoObserver*> observers_;
};

// Implements the info object for server selections
class VInfoServer : public VInfo, public std::enable_shared_from_this<VInfo>
{
public:
	bool isServer() {return true;}
    bool isEmpty() {return false;}
    bool hasData() const;
    VItem* item() const;

    void accept(VInfoVisitor*);   
    std::string name();
    std::string path();
    std::string nodePath() {return "/";}
    static VInfo_ptr create(ServerHandler*);

protected:
    explicit VInfoServer(ServerHandler*);
};


// Implements the info object for node selections
class VInfoNode: public VInfo, public std::enable_shared_from_this<VInfo>
{
public:
	bool isNode() {return true;}
	bool isEmpty() {return false;}
    bool hasData() const;
    void accept(VInfoVisitor*);
    std::string path();  
	std::string name();	
	std::string serverAlias();
    std::string nodePath();
    std::string relativePath();
    VItem* item() const;

    static VInfo_ptr create(VNode*);

protected:
	VInfoNode(ServerHandler*,VNode*);
};


// Implements the info  base class for attribute selections
class VInfoAttribute: public VInfo, public std::enable_shared_from_this<VInfo>
{
public:
    ~VInfoAttribute();
    VAttribute* attribute() const {return attr_;}
    bool isAttribute() {return true;}
	bool isEmpty() {return false;}
    bool hasData() const;
    void accept(VInfoVisitor*);
    std::string name();
    std::string path();
    std::string nodePath();
    VItem* item() const;

    static VInfo_ptr create(VAttribute*);

protected:
    VInfoAttribute(ServerHandler*,VNode*,VAttribute*);
};

typedef std::shared_ptr<VInfoServer>   VInfoServer_ptr;
typedef std::shared_ptr<VInfoNode>   VInfoNode_ptr;
typedef std::shared_ptr<VInfoAttribute>   VInfoAttribute_ptr;

class VInfoVisitor
{
public:
    VInfoVisitor() = default;
    virtual ~VInfoVisitor() = default;

	virtual void visit(VInfoServer*)=0;
	virtual void visit(VInfoNode*)=0;
	virtual void visit(VInfoAttribute*)=0;

};

class VInfoObserver
{
public:
    VInfoObserver() = default;
    virtual ~VInfoObserver() = default;

	virtual void notifyDataLost(VInfo*)=0;
	virtual void notifyDelete(VInfo*)=0;
};

#endif

