//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "VInfo.hpp"

#include "ServerHandler.hpp"
#include "UiLog.hpp"
#include "VAttribute.hpp"
#include "VAttributeType.hpp"
#include "VItemPathParser.hpp"
#include "VNode.hpp"

#include <boost/lexical_cast.hpp>

//#define _UI_VINFO_DEBUG

//========================================
//
// VInfo
//
//========================================

VInfo::VInfo(ServerHandler* server,VNode* node,VAttribute* attr) :
	server_(server),
    node_(node),
    attr_(attr)
{   
	if(server_)
		server_->addServerObserver(this);
}

VInfo::~VInfo()
{
#ifdef _UI_VINFO_DEBUG
    UiLog().dbg() << "VInfo::~VInfo() --> " << this;
#endif
    if(server_)
		server_->removeServerObserver(this);

	for(std::vector<VInfoObserver*>::const_iterator it=observers_.begin(); it != observers_.end(); ++it)
		(*it)->notifyDelete(this);

#ifdef _UI_VINFO_DEBUG
    UiLog().dbg() << "<-- ~VInfo()";
#endif
}

void VInfo::notifyServerDelete(ServerHandler* /*server*/)
{
    server_=0;
    node_=0;
    attr_=0;

    //This function is called from the server destructor. We do not remove this object from the ServerObservers
	dataLost();
}

void VInfo::dataLost()
{
    std::vector<VInfoObserver*> obsTmp=observers_;
	observers_.clear();

	for(std::vector<VInfoObserver*>::iterator it=obsTmp.begin(); it != obsTmp.end(); ++it)
	{
		VInfoObserver* o=*it;
		o->notifyDataLost(this);
    }

    attr_=0;
}

void VInfo::notifyBeginServerClear(ServerHandler* server)
{    
    assert(server_==server);
    node_=0;
    attr_=0;
}

void VInfo::notifyEndServerClear(ServerHandler* server)
{
    assert(server_==server);
    node_=0;   
    attr_=0;
}

void VInfo::notifyEndServerScan(ServerHandler* server)
{
    assert(server_==server);
    regainData();
}

void VInfo::regainData()
{
    if(!server_)
    {
        dataLost();
        return;
    }

    if(!node_)
    {
        if(isServer())
        {
            node_=server_->vRoot();
            return;
        }
        else if(isNode())
        {
            VItemPathParser p(storedPath_);
            if(p.itemType() == VItemPathParser::NodeType)
            {
                node_=server_->vRoot()->find(p.node());
                if(node_)
                    return;
            }
            if(!node_)
            {
                dataLost();
                return;
            }
        }
     }

     if(isAttribute())
     {
        VItemPathParser p(storedPath_);
        if(p.itemType() == VItemPathParser::AttributeType)
        {
            if(!node_)
            {
                node_=server_->vRoot()->find(p.node());
            }
            if(node_)
            {
                attr_=node_->findAttribute(p.type(),p.attribute());
            }
            if(!node_ || !attr_)
            {
                dataLost();
            }
        }
    }
}

std::string VInfo::storedNodePath() const
{
     VItemPathParser p(storedPath_);
     if(p.itemType() == VItemPathParser::ServerType)
         return "/";
     else
         return p.node();
}

void VInfo::addObserver(VInfoObserver* o)
{
	std::vector<VInfoObserver*>::iterator it=std::find(observers_.begin(),observers_.end(),o);
	if(it == observers_.end())
		observers_.push_back(o);
}

void VInfo::removeObserver(VInfoObserver* o)
{
	std::vector<VInfoObserver*>::iterator it=std::find(observers_.begin(),observers_.end(),o);
	if(it != observers_.end())
		observers_.erase(it);
}

bool VInfo::operator ==(const VInfo& other)
{
    if(server_ == other.server_ && node_ == other.node_ &&
            storedPath_ == other.storedPath_)
    {
        if((!attr_ && other.attr_) ||
           (attr_ && !other.attr_))
            return false;

        else if(attr_ && other.attr_)
        {
            return (attr_->type() == other.attr_->type() &&
                    attr_->data() == other.attr_->data());
        }
        else
            return true;
    }
    return false;
}

VInfo_ptr VInfo::createParent(VInfo_ptr info)
{
    if(!info)
        return VInfo_ptr();

    if(info->isServer())
        return info;
    else if(info->isNode())
    {
        return VInfoServer::create(info->server());
    }
    else if(info->isAttribute())
    {
        return VInfoNode::create(info->node());
    }

    return VInfo_ptr();
}


VInfo_ptr VInfo::createFromPath(ServerHandler* s,const std::string& path)
{
    if(!s || path.empty())
        return VInfo_ptr();

    VItemPathParser p(path);

    if(p.itemType() ==  VItemPathParser::ServerType)
    {
        return VInfoServer::create(s);
    }
    else if(p.itemType() ==  VItemPathParser::NodeType)
    {
        if(VNode* n=s->vRoot()->find(p.node()))
            return VInfoNode::create(n);

    }
    else if(p.itemType() ==  VItemPathParser::AttributeType)
    {
        if(VNode* n=s->vRoot()->find(p.node()))
        {
            if(VAttribute* a=n->findAttribute(p.type(),p.attribute()))
            {
                return VInfoAttribute::create(a);
            }           
        }
    }

    return VInfo_ptr();
}

VInfo_ptr VInfo::createFromPath(const std::string& path)
{
    if(path.empty())
        return VInfo_ptr();

    VItemPathParser p(path);
    if(!p.server().empty())
    {
        if(ServerHandler* s=ServerHandler::find(p.server()))
        {
            return createFromPath(s,path);
        }
    }
    return VInfo_ptr();
}

VInfo_ptr VInfo::createFromItem(VItem* item)
{
    if(!item)
        return VInfo_ptr();

    if(VServer* s=item->isServer())
    {
        return VInfoServer::create(s->server());
    }
    else if(VNode* n=item->isNode())
    {
        return VInfoNode::create(n);
    }
    else if(VAttribute* a=item->isAttribute())
    {
        return VInfoAttribute::create(a);
    }

    return VInfo_ptr();
}

//=========================================
//
// VInfoServer
//
//=========================================

VInfoServer::VInfoServer(ServerHandler *server) : VInfo(server,NULL)
{
	if(server_)
	{
		node_=server_->vRoot();
        storedPath_=VItemPathParser::encodeWithServer(server_->name(),"/","server");
	}
}

VInfo_ptr VInfoServer::create(ServerHandler *server)
{
	return VInfo_ptr(new VInfoServer(server));
}

bool VInfoServer::hasData() const
{
    return server_ != 0;
}

void VInfoServer::accept(VInfoVisitor* v)
{
	v->visit(this);
}

std::string VInfoServer::name()
{
    return (server_)?(server_->name()):(std::string());
}

std::string VInfoServer::path()
{
    return name() + "://";
}

VItem* VInfoServer::item() const
{
    return node_;
}

//=========================================
//
// VInfoNode
//
//=========================================


VInfoNode::VInfoNode(ServerHandler* server,VNode* node) : VInfo(server,node)
{
    if(node_)
    {
        assert(server_);
        storedPath_=VItemPathParser::encodeWithServer(server_->name(),node_->absNodePath(),"node");
    }
}

VInfo_ptr VInfoNode::create(VNode *node)
{
	ServerHandler* server=NULL;
	if(node)
	{
		server=node->server();
	}
	return VInfo_ptr(new VInfoNode(server,node));
}

bool VInfoNode::hasData() const
{
    return server_ != 0 && node_ != 0;
}

void VInfoNode::accept(VInfoVisitor* v)
{
	v->visit(this);
}

std::string VInfoNode::name()
{
	if(node_ && node_->node())
		return node_->strName();

	return std::string();
}

std::string VInfoNode::path()
{
    std::string p;
    if(server_)
       p=server_->name();

    if(node_ && node_->node())
        p+=":/" + node_->absNodePath();

    return p;
}

std::string VInfoNode::serverAlias()
{
    std::string p;
    if(server_)
       p = server_->name();
    return p;
}

std::string VInfoNode::nodePath()
{
    std::string p;
    if(node_ && node_->node())
        p = node_->absNodePath();
    return p;
}

std::string VInfoNode::relativePath()
{
    std::string p;
    if(node_ && node_->node())
        p = node_->absNodePath();
    return p;
}

VItem* VInfoNode::item() const
{
    return node_;
}

//=========================================
//
// VInfoAttribute
//
//=========================================


VInfoAttribute::VInfoAttribute(ServerHandler* server,VNode* node,VAttribute* attr) :
        VInfo(server,node,attr)
{
    if(attr_)
    {
        assert(server_);
        storedPath_=VItemPathParser::encodeWithServer(server_->name(),
                                                      attr_->fullPath(),attr_->typeName());
    }
}

VInfoAttribute::~VInfoAttribute()
{
}

bool VInfoAttribute::hasData() const
{
    return server_ != 0 && node_ != 0 && attr_ != 0;
}

void VInfoAttribute::accept(VInfoVisitor* v)
{
	v->visit(this);
}

VInfo_ptr VInfoAttribute::create(VAttribute* att)
{
    ServerHandler* server=NULL;
    VNode* node=att->parent();
    if(node)
    {
        server=node->server();
    }

    return VInfo_ptr(new VInfoAttribute(server,node,att));
}

std::string VInfoAttribute::path()
{
    std::string p;
    if(server_)
       p=server_->name();
    if(attr_)
        p+=attr_->fullPath();

    return p;
}

std::string VInfoAttribute::nodePath()
{
    std::string p;
    if(node_ && node_->node())
        p = node_->absNodePath();
    return p;
}

std::string VInfoAttribute::name()
{
    return (attr_)?attr_->strName():std::string();
}

VItem* VInfoAttribute::item() const
{
    return attr_;
}









