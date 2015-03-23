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

#include "Node.hpp"
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

VInfoAttribute* VInfoAttributeFactory::create(VAttribute* att,int attIndex,Node* node,ServerHandler* server)
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

VInfo::VInfo() :
	server_(0),
	node_(0),
	att_(0),
	attIndex_(0)
{

}

VInfo::VInfo(ServerHandler* server) :
	server_(server),
	node_(0),
	att_(0),
	attIndex_(0)
{

}

VInfo::VInfo(Node* node,ServerHandler* server) :
	server_(server),
	node_(node),
	att_(0),
	attIndex_(0)
{

}

VInfo::VInfo(VAttribute* att,int attIndex,Node* node,ServerHandler* server) :
	server_(server),
	node_(node),
	att_(att),
	attIndex_(attIndex)
{

}

void VInfo::ancestors(ServerHandler **sv,std::vector<Node*>& nodes)
{
	*sv=server();

	if(isNode())
	{
		Node* n=node();
		while(n)
		{

			nodes.push_back(n);
			n=n->parent();
		}
	}
}

bool VInfo::sameAs(Node* n,bool checkAncestors)
{
    if(isNode())
    {
        if(n == node())
            return true;
        
        if(checkAncestors)
        {
            Node* nd=node();
            while(nd)
            {
                if(n == nd)
                    return true;
           
                nd=nd->parent();
            }    
        }
    }
    
    return false;
}

//--------------------------------------------------
// Factory methods
//--------------------------------------------------

VInfo* VInfo::make(ServerHandler* s)
{
	return new VInfoServer(s);
}


VInfo* VInfo::make(Node* n,ServerHandler* s)
{
	return new VInfoNode(n,s);
}

VInfo* VInfo::make(VAttribute* att,int attIndex,Node* node)
{
	return VInfoAttributeFactory::create(att,attIndex,node);
}

//=========================================
//
// VInfoServer
//
//=========================================

VInfoServer::VInfoServer(ServerHandler *server) : VInfo(server)
{

}

void VInfoServer::accept(VInfoVisitor* v)
{
	v->visit(this);
}

void VInfoServer::variables(std::vector<Variable>& vars)
{
	ServerDefsAccess defsAccess(server_);  // will reliquish its resources on destruction
	const std::vector<Variable>& v=defsAccess.defs()->server().server_variables();

	vars=v;
}

std::string VInfoServer::name()
{
	if(server_)
		return server_->longName();

	return std::string();
}

/*
VInfoReply_ptr VInfoServer::info(VInfoQuery_ptr q)
{
	if(!server_)
	{
		//invoker_->queryFailed();
		return 0;
	}

	VInfoQuery_ptr tmpQuery(new VInfoQuery(VInfoRequest::STATS,this));
	//queue_[VInfoQuery::STATS] << query;

	server_->query(tmpQuery);

	queue_.push_back(make_pair(tmpQuery,q));

	VInfoReply_ptr reply(new VInfoReply(q));
	return reply;
}

void VInfoServer::info(VInfoReply_ptr r)
{
	if(!r || r->type() != VInfoQuery::STATS)
		return;

	std::stringstream ss;

	ss << r->text();
	info(ss);
	r->text(ss.str());



	query_->sender()->queryFinished(r);
}



void VInfoServer::info(std::stringstream& f)
{
	if(!server_) return;

	static const std::string inc = "  ";

	using namespace boost::posix_time;
	using namespace boost::gregorian;

	std::string typeName="server";
	std::string nodeName=server_->name();
	std::string statusName(VSState::toName(server_).toStdString());

	//Header
	f << "name    : " << nodeName << "\n";
	f << "type    : " << typeName << "\n";
	f << "status   : " << statusName << "\n";

    f << "----------\n";
    //Start block: Type, name
    f << typeName << " " << server_->name() << "\n";

    //End block
    f << "end" << typeName << " # " << nodeName << "\n";
}
*/
//=========================================
//
// VInfoNode
//
//=========================================


VInfoNode::VInfoNode(Node *node,ServerHandler *server) : VInfo(node,server)
{

}

void VInfoNode::accept(VInfoVisitor* v)
{
	v->visit(this);
}

ServerHandler* VInfoNode::server()
{
	if(server_ == NULL && node_)
	{
		server_=ServerHandler::find(node_);
	}
	return server_;
}

const std::string&  VInfoNode::nodeType()
{
	return nodeType(node_);
}

const std::string&  VInfoNode::nodeType(Node* node)
{
	static std::string suiteStr("suite");
	static std::string familyStr("family");
	static std::string taskStr("task");
	static std::string defaultStr("node");

	if(!node)
		return defaultStr;

	if(node->isSuite())
		return suiteStr;
	else if(node->isFamily())
		return familyStr;
	else if(node->isTask())
			return taskStr;

	return defaultStr;
}


void VInfoNode::variables(std::vector<Variable>& vars)
{
	vars=node_->variables();
}

void VInfoNode::genVariables(std::vector<Variable>& genVars)
{
	genVars.clear();
	node_->gen_variables(genVars);
}

std::string VInfoNode::name()
{
	if(node_)
		return node_->name();

	return std::string();
}

/*
void VInfoNode::info(VInfoReplyReceiver* invoker)
{
	if(invoker) return;

	VInfoReply_ptr reply(new VInfoReply(VInfoRequest::INFO,invoker));

	std::stringstream ss;
	info(ss);
	reply->text(ss.str());
	reply->done(true);
	reply->owner()->queryFinished(reply);
}

//Re-implemented from simple_node.cc
//simple_node::info(std::ostream& f)

void VInfoNode::info(std::stringstream& f)
{
	if(!node_) return;

	static const std::string inc = "  ";

	using namespace boost::posix_time;
	using namespace boost::gregorian;

	std::string typeName=nodeType(node_);
	std::string nodeName=node_->name();
	std::string statusName(VNState::toName(node_).toStdString());

	//Header
	f << "name    : " << nodeName << "\n";
	f << "type    : " << typeName << "\n";
	f << "status   : " << statusName << "\n";

	boost::posix_time::ptime state_change_time = node_->state_change_time();
    if(!state_change_time.is_special())
    {
    	f << "at      : " << boost::posix_time::to_simple_string(state_change_time) << "\n";
	}

    f << "----------\n";

    //Start block: Type, name
	f << typeName << " " << node_->name() << "\n";

	//Clock information for suites
    if(node_->isSuite())
    {
    	Suite* suite = dynamic_cast<Suite*>(node_);
    	// f << "clock    : ";
    	if (suite->clockAttr()) {
    		suite->clockAttr().get()->print(f); // f << "\n";
    	}
    }

    //Default status: the status the node should have when the begin/re-queue is called
    //if(st  != DState::QUEUED && st != DState::UNKNOWN)
    f << inc << "defstatus " <<  VNState::toDefaultStateName(node_).toStdString() << "\n";

    //Zombies attribute
    const std::vector<ZombieAttr> & vect = node_->zombies();
    for (std::vector<ZombieAttr>::const_iterator it = vect.begin(); it != vect.end(); ++it)
    		f << inc << it->toString() << "\n";

    //Autocancel
    if(node_->hasAutoCancel() && node_->get_autocancel())
          f << inc << node_->get_autocancel()->toString() << "\n";

    f << inc << "# " << typeName << " " << nodeName << " is " << statusName << "\n";

    if(node_->hasTimeDependencies())
    {
	      f << inc << "# time-date-dependencies: ";
	      if (node_->isTimeFree()) f << "free\n";
	      else f << "holding\n";
    }

    //Generated variables
    std::vector<Variable> gvar;
    std::vector<Variable>::const_iterator gvar_end;
    node_->gen_variables(gvar);
    for(std::vector<Variable>::const_iterator it = gvar.begin(); it != gvar.end(); ++it)
    {
    	f << inc << "# edit " << (*it).name() << " '" << (*it).theValue() << "'\n";
    }

    //Variables
    gvar = node_->variables();
    for(std::vector<Variable>::const_iterator it = gvar.begin(); it != gvar.end(); ++it)
    {
	      f << inc << "edit " << (*it).name() << " '" << (*it).theValue() << "'\n";
    }


    //Print children
    std::vector<node_ptr> nodes;
    node_->immediateChildren(nodes);
    for(unsigned int i=0; i < nodes.size(); i++)
    {
    	f << inc << nodeType(nodes.at(i).get()) << " " << nodes.at(i)->name() << "\n";
	}

    //Here we should print some additional information from the attributes well. It i not clear exactly what!

    //End block
    f << "end" << typeName << " # " << nodeName << "\n";
}
*/

//=========================================
//
// VInfoAttribute
//
//=========================================


VInfoAttribute::VInfoAttribute(VAttribute* att,int attIndex,Node* node,ServerHandler *server) :
		VInfo(att,attIndex,node,server)
{

}

void VInfoAttribute::accept(VInfoVisitor* v)
{
	v->visit(this);
}


VInfoLimit::VInfoLimit(VAttribute* att,int attIndex,Node* node,ServerHandler *server) :
		VInfoAttribute(att,attIndex,node,server)
{

}

static VInfoAttributeMaker<VInfoLimit> maker1("limit");



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











