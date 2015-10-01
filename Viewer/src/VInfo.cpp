//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "VInfo.hpp"

#include "VNode.hpp"
#include "Suite.hpp"

#include "ServerHandler.hpp"
#include "VAttribute.hpp"
#include "VNState.hpp"
#include "VSState.hpp"

static std::map<std::string,VInfoAttributeFactory*>* makers = 0;

//========================================
//
// VInfoAttributeFactory
//
//========================================


VInfoAttributeFactory::VInfoAttributeFactory(const std::string& name)
{
	if(makers == 0)
		makers = new std::map<std::string,VInfoAttributeFactory*>;

	// Put in reverse order...
	(*makers)[name] = this;
}

VInfoAttributeFactory::~VInfoAttributeFactory()
{
	// Not called
}

VInfoAttribute* VInfoAttributeFactory::create(VAttribute* att,int attIndex,VNode* node,ServerHandler* server)
{
	std::string name=att->name().toStdString();

	std::map<std::string,VInfoAttributeFactory*>::iterator j = makers->find(name);
	if(j != makers->end())
		return (*j).second->make(att,attIndex,node,server);

	//Default
	//return  new MvQTextLine(e,p);
	//return new MvQLineEditItem(e,p) ;
	return 0;
}


//========================================
//
// VInfo
//
//========================================

VInfo::VInfo(ServerHandler* server,VNode* node) :
	server_(server),
	node_(node)
{
	if(node)
		nodePath_=node->absNodePath();

	if(server_)
		server_->addServerObserver(this);
}

VInfo::~VInfo()
{
	if(server_)
		server_->removeServerObserver(this);

	for(std::vector<VInfoObserver*>::const_iterator it=observers_.begin(); it != observers_.end(); ++it)
		(*it)->notifyDelete(this);
}

void VInfo::notifyServerDelete(ServerHandler* server)
{
	//This function is called from the server destructor. We do not remove this object from the ServerObservers

	server_=NULL;
	node_=NULL;

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
}

void VInfo::notifyBeginServerClear(ServerHandler* server)
{
	node_=NULL;
}

void VInfo::notifyEndServerScan(ServerHandler* server)
{
	if(isNode())
	{
		node_=server_->vRoot()->find(nodePath_);
		if(!node_)
		{
			dataLost();
		}
	}
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
		//server_->addServerObserver(this);
	}
}

VInfo_ptr VInfoServer::create(ServerHandler *server)
{
	return VInfo_ptr(new VInfoServer(server));
}

void VInfoServer::accept(VInfoVisitor* v)
{
	v->visit(this);
}

std::string VInfoServer::name()
{
	if(server_)
		return server_->name();

	return std::string();
}

std::string VInfoServer::path()
{
    return name() + ":/";
}

//=========================================
//
// VInfoNode
//
//=========================================


VInfoNode::VInfoNode(ServerHandler* server,VNode* node) : VInfo(server,node)
{
	//if(server_)
	//	server_->addServerObserver(this);
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

//=========================================
//
// VInfoAttribute
//
//=========================================


VInfoAttribute::VInfoAttribute(ServerHandler* server,VNode* node,VAttribute* att,int attIndex) :
		VInfo(server,node),
		att_(att),
		attIndex_(attIndex)
{

}

void VInfoAttribute::accept(VInfoVisitor* v)
{
	v->visit(this);
}

VInfo_ptr VInfoAttribute::create(ServerHandler* server,VNode* node,VAttribute* att,int attIndex)
{
	return VInfo_ptr(new VInfoAttribute(server,node,att,attIndex));
}



/*
VInfoLimit::VInfoLimit(VAttribute* att,int attIndex,VNode* node,ServerHandler *server) :
		VInfoAttribute(att,attIndex,node,server)
{

}

static VInfoAttributeMaker<VInfoLimit> maker1("limit");
*/


/*static VMeterAttribute meterAttr("meter");
static VEventAttribute eventAttr("event");
static VRepeatAttribute repeatAttr("repeat");
static VTriggerAttribute triggerAttr("trigger");
static VLabelAttribute labelAttr("label");
static VTimeAttribute timeAttr("time");
static VDateAttribute dateAttr("date");
static VLimitAttribute limitAttr("limit");
static VLimiterAttribute limiterAttr("limiter");
static VLateAttribute lateAttr("late");
static VVarAttribute varAttr("var");
static VGenvarAttribute genvarAttr("genvar");*/











