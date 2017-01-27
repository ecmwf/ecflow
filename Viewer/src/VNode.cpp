//============================================================================
// Copyright 2016 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "VNode.hpp"

#include <QDebug>

#include "Node.hpp"
#include "Variable.hpp"

#include "AstCollateVNodesVisitor.hpp"
#include "ConnectState.hpp"
#include "ServerDefsAccess.hpp"
#include "ServerHandler.hpp"
#include "TriggerCollector.hpp"
#include "TriggeredScanner.hpp"
#include "VAttribute.hpp"
#include "VAttributeType.hpp"
#include "VDateAttr.hpp"
#include "VEventAttr.hpp"
#include "VFileInfo.hpp"
#include "VFilter.hpp"
#include "VLabelAttr.hpp"
#include "VLateAttr.hpp"
#include "VLimitAttr.hpp"
#include "VLimiterAttr.hpp"
#include "VMeterAttr.hpp"
#include "VNState.hpp"
#include "VSState.hpp"
#include "VTaskNode.hpp"
#include "VTimeAttr.hpp"
#include "VTriggerAttr.hpp"
#include "VGenVarAttr.hpp"
#include "VUserVarAttr.hpp"

#include <boost/algorithm/string.hpp>

#define _UI_VNODE_DEBUGconst std::vector<Meter>& v=node->meters();


// static member of VNode
std::string VNode::nodeMarkedForMoveRelPath_;
std::string VNode::nodeMarkedForMoveServerAlias_;


//For a given node this class stores all the nodes that this node itself triggers.
//For memory efficiency we only store the AttributeFilterindex of the nodes not the pointers themselves.
class VNodeTriggerData
{
public:
    std::vector<int> data_;

    void get(VNode* node ,TriggerCollector* tc)
    {
        VServer* s=node->root();
        for(size_t i=0; i < data_.size(); i++)
        {
            VItemTmp_ptr triggered(VItemTmp::create(s->nodeAt(data_[i])));
            VItemTmp_ptr nullItem;
            tc->add(triggered,nullItem,TriggerCollector::Normal);
        }
    }
    void add(VItem* n)
    {
        assert(n);
        VNode* node=n->isNode();
        assert(node);
        data_.push_back(node->index());
    }

    void add(VItem* /*triggered*/,VItem* /*trigger*/)
    {
    }
};

#if 0
class VNodeTriggerData
{
public:
    std::vector<std::pair<int,int> > data_;

    void get(VNode* node ,TriggerCollector* tc)
    {
        VServer* s=node->root();

        for(size_t i=0; i < data_.size(); i++)
        {
            VItemTmp_ptr triggered(VItemTmp::create(s->nodeAt(data_[i].first)));
            VItemTmp_ptr trigger;
            if(data_[i].second == -2)
            {
                trigger=VItemTmp::creaate(node);
            else
            {
                trigger=VItemTmp::create(VAttribute::makeFromId(node,data_[i].second));
            }

            tc->add(triggered,0,TriggerCollector::Normal,trigger);
            VItemTmp_ptr nullItem;
            tc->add(triggered,nullItem,TriggerCollector::Normal);
        }
    }
AttributeFilter
    void add(VItem* n)
    {
        assert(n);
        VNode* node=n->isNode();
        assert(node);
        data_.push_back(std::make_pair(node->index(),-2));
    }

    void add(VItem* triggered,VItem* trigger)
    {
        assert(triggered);
        assert(trigger);
        VNode* node=triggered->isNode();
        assert(node);
        VAttribute* attr=trigger->isAttribute();
        assert(attr);
        if(attr->id() >=0)
            data_.push_back(std::make_pair(node->index(),attr->id()));
    }
};
#endif

//=================================================
// VNode
//=================================================

VNode::VNode(VNode* parent,node_ptr node) :
    VItem(parent),
    node_(node),
    //parent_(parent),
#if 0
    attrNum_(-1),
    cachedAttrNum_(-1),
#endif
    index_(-1),
    data_(NULL)
{
	if(parent_)
		parent_->addChild(this);

	if(node_)
		node_->set_graphic_ptr(this);

    scanAttr();
}

VNode::~VNode()
{
    if(data_)
        delete data_;

    for(size_t i=0; i < attr_.size(); i++)
        delete attr_[i];
}

VServer* VNode::root() const
{
    return server()->vRoot();
}

ServerHandler* VNode::server() const
{
	return (parent_)?(parent_->server()):NULL;
}

VNode* VNode::suite() const
{
    if(isTopLevel())
        return const_cast<VNode*>(this);

    VNode* p=parent();
    while(p)
    {
        if(p->isTopLevel())
            return p;
        p=p->parent();
    }

    assert(0);

    return NULL;
}

bool VNode::isTopLevel() const
{
    return isSuite();
    //return (parent_ && parent_->isServer());
	//return (node_)?(node_->isSuite() != NULL):false;
}

void VNode::clear()
{
	children_.clear();
#if 0
    attrNum_=-1,
	cachedAttrNum_=-1;
#endif
}

bool VNode::hasAccessed() const
{
	return true; //!name_.empty();
}

#if 0
//At the beginning of the update we get the current number of attributes
void VNode::beginUpdateAttrNum()
{
	//if(attrNum_ != -1)
	//{
	//	attrNum_=cachedAttrNum_;
	//}

	//attrNum_=VAttribute::totalNum(node_);
}

//At the end of the update we set the cached value to the current number of attributes
void VNode::endUpdateAttrNum()
{
	cachedAttrNum_=attrNum_;
    attrNum_=VAttributeType::totalNum(this);
}

short VNode::cachedAttrNum() const
{
	return cachedAttrNum_;
}
#endif


void VNode::scanAttr()
{
    VLabelAttr::scan(this,attr_);
    VMeterAttr::scan(this,attr_);
    VEventAttr::scan(this,attr_);
    VTriggerAttr::scan(this,attr_);
    VLimitAttr::scan(this,attr_);
    VLimiterAttr::scan(this,attr_);
    VLateAttr::scan(this,attr_);
    VTimeAttr::scan(this,attr_);
    VDateAttr::scan(this,attr_);
    VUserVarAttr::scan(this,attr_);
    VGenVarAttr::scan(this,attr_);
}

void VNode::rescanAttr()
{
    for(size_t i=0; i < attr_.size(); i++)
        delete attr_[i];

    attr_.clear();
    scanAttr();
}

int VNode::attrNum(AttributeFilter *filter) const
{     
    if(filter)
    {
        int n=0;
        for(size_t i=0; i < attr_.size(); i++)
        {
            if(filter->isSet(attr_[i]->type()))
                n++;
        }
        return n;
    }

    return attr_.size();

    //return VAttributeType::totalNum(this,filter);

#if 0
    //If if was not initialised we get its value
	if(attrNum_==-1)
	{
        attrNum_=VAttributeType::totalNum(this);

		if(cachedAttrNum_ == -1)
			cachedAttrNum_=attrNum_;
	}

	return attrNum_;
#endif

}

#if 0
short VNode::currentAttrNum() const
{
    return VAttributeType::totalNum(this);
}
#endif

VAttribute* VNode::attribute(int row,AttributeFilter *filter) const
{
    assert(row>=0);

    if(filter)
    {
        int n=0;
        int cnt=attr_.size();
        if(row >= cnt)
            return 0;

        for(size_t i=0; i < cnt; i++)
        {
            if(filter->isSet(attr_[i]->type()))
            {
                if(n == row)
                {
                    return attr_[i];
                }
                n++;
            }
        }
    }
    else if(row < attr_.size())
    {
        return attr_[row];
    }

    return 0;
}

int VNode::attributeIndex(const VAttribute* a, AttributeFilter *filter) const
{
    if(filter)
    {
        int n=0;
        int cnt=attr_.size();
        for(size_t i=0; i < cnt; i++)
        {
            if(filter->isSet(attr_[i]->type()))
            {
                if(a==attr_[i])
                    return n;

                n++;
            }
        }
    }
    else
    {
        int n=0;
        int cnt=attr_.size();
        for(size_t i=0; i < cnt; i++)
        {
            if(a==attr_[i])
                return i;
        }
    }

    return -1;
}

QStringList VNode::getAttributeData(int row,VAttributeType*& type)
{
    return attr_[row]->data();
#if 0
    QStringList lst;
    VAttributeType::getData(this,row,type,lst);
	return lst;
#endif
}

QStringList VNode::getAttributeData(int row,AttributeFilter *filter)
{
    if(VAttribute *a=attribute(row,filter))
        return a->data();

    return QStringList();

#if 0
    VAttributeType* type;
    QStringList lst;
    VAttributeType::getData(this,row,type,lst,filter);
    return lst;
#endif
}

#if 0
VAttributeType* VNode::getAttributeType(int row)
{
    return VAttributeType::getType(this,row);
}
#endif

bool VNode::getAttributeData(const std::string& type,int row,QStringList& data)
{
#if 0
    return VAttributeType::getData(type,this,row,data);
#endif
}

int VNode::getAttributeLineNum(int row,AttributeFilter *filter)
{
    return attr_[row]->lineNum();
    //return VAttributeType::getLineNum(this,row,filter);
}

QString VNode::attributeToolTip(int row,AttributeFilter *filter)
{
    if(VAttribute *a=attribute(row,filter))
        return a->toolTip();

    return QString();

#if 0
    VAttributeType* type;
    QStringList lst;
    VAttributeType::getData(this,row,type,lst,filter);
    return (type)?(type->toolTip(lst)):QString();
#endif
}

void VNode::addChild(VNode* vn)
{
	children_.push_back(vn);
}

void VNode::removeChild(VNode* vn)
{
	std::vector<VNode*>::iterator it=std::find(children_.begin(),children_.end(),vn);
	if(it != children_.end())
	{
		children_.erase(it);
	}
}

VNode* VNode::childAt(int index) const
{
    assert(index>=0 && index < children_.size());
    return children_[index];
}

int VNode::indexOfChild(const VNode* vn) const
{
	for(unsigned int i=0; i < children_.size(); i++)
	{
        if(children_[i] == vn)
			return i;
	}

	return -1;
}

int VNode::indexOfChild(node_ptr n) const
{
	for(unsigned int i=0; i < children_.size(); i++)
	{
        if(children_[i]->node() == n)
			return i;
    }

	return -1;
}

VNode *VNode::findChild(const std::string& name) const
{
	for(unsigned int i=0; i < children_.size(); i++)
	{
        if(children_[i]->sameName(name))
			return children_.at(i);
	}
	return 0;
}

void VNode::collect(std::vector<VNode*>& vec) const
{
	for(int i=0; i < numOfChildren(); i++)
	{
		vec.push_back(children_.at(i));
        children_[i]->collect(vec);
	}
}

int VNode::tryNo() const
{
	std::string v=genVariable("ECF_TRYNO");
	if(v.empty())
		return 0;

	return boost::lexical_cast<int>(v);
}


VNode* VNode::find(const std::vector<std::string>& pathVec)
{
	if(pathVec.size() == 0)
		return this;

	if(pathVec.size() == 1)
	{
		return findChild(pathVec.at(0));
	}

	std::vector<std::string> rest(pathVec.begin()+1,pathVec.end());
	VNode *n = findChild(pathVec.at(0));

	return n?n->find(rest):NULL;
}

std::string VNode::genVariable(const std::string& key) const
{
    std::string val;
    if(node_ )
    	node_->findGenVariableValue(key,val);
    return val;
}

std::string VNode::findVariable(const std::string& key,bool substitute) const
{
	std::string val;
	if(!node_ )
	    return val;

	const Variable& var=node_->findVariable(key);
	if(!var.empty())
	{
		val=var.theValue();
		if(substitute)
		{
		    node_->variableSubsitution(val);
		}
		return val;
	}
	const Variable& gvar=node_->findGenVariable(key);
	if(!gvar.empty())
	{
	   val=gvar.theValue();
	   if(substitute)
	   {
		   node_->variableSubsitution(val);
	   }
	   return val;
	}

	return val;
}

std::string VNode::findInheritedVariable(const std::string& key,bool substitute) const
{
    std::string val;
    if(!node_ )
    	return val;

    const Variable& var=node_->findVariable(key);
    if(!var.empty())
    {
    	val=var.theValue();
    	if(substitute)
    	{
    		node_->variableSubsitution(val);
    	}
    	return val;
    }
    const Variable& gvar=node_->findGenVariable(key);
    if(!gvar.empty())
    {
    	val=gvar.theValue();
       	if(substitute)
       	{
       		node_->variableSubsitution(val);
       	}
       	return val;
    }

    //Try to find it in the parent
    if(parent())
    {
    	return parent()->findInheritedVariable(key,substitute);
    }

    return val;
}

int VNode::variablesNum() const
{
	if(node_.get())
		return static_cast<int>(node_->variables().size());

	return 0;
}

int VNode::genVariablesNum() const
{
    std::vector<Variable> gv;

    if(node_)
	{
		node_->gen_variables(gv);
		return static_cast<int>(gv.size());
	}

	return 0;
}

void VNode::variables(std::vector<Variable>& vars) const
{
	vars.clear();
    if(node_)
		vars=node_->variables();
}

void VNode::genVariables(std::vector<Variable>& genVars) const
{
	genVars.clear();
    if(node_)
		node_->gen_variables(genVars);
}

std::string VNode::fullPath() const
{
    return absNodePath();
}

std::string VNode::absNodePath() const
{
	return (node_)?node_->absNodePath():"";
}


void VNode::setNodeMarkedForMove(std::string serverAlias, std::string relPath)
{
	nodeMarkedForMoveServerAlias_ = serverAlias;
	nodeMarkedForMoveRelPath_ = relPath;
}

void VNode::clearNodeMarkedForMove()
{
	nodeMarkedForMoveRelPath_.clear();
	nodeMarkedForMoveServerAlias_.clear();
}


bool VNode::sameContents(VItem* item) const
{
    return item == this;
}

bool VNode::sameName(const std::string& name) const
{
	return(strName() == name)?true:false;
	//return (node_)?(node_->name() == name):false;
}

std::string VNode::strName() const
{
	if(node_ && node_.get())
		return node_->name();

	return std::string();
	/*
	if(name_.empty())
	{
		if(node_)
			name_=node_->name();
	}
	return name_;*/
}

QString VNode::name() const
{
	return QString::fromStdString(strName());
	//return (node_)?QString::fromStdString(node_->name()):QString();
}

std::string VNode::serverName() const
{
	if(ServerHandler* s=server())
	{
		return s->name();
	}
	return std::string();
}

QString VNode::stateName()
{
	return VNState::toName(this);
}

bool VNode::isDefaultStateComplete()
{
	if(node_)
		return (node_->defStatus() == DState::COMPLETE);

	return false;
}

QString VNode::defaultStateName()
{
	return VNState::toDefaultStateName(this);
}

QString VNode::serverStateName()
{
	return QString("");
}

bool VNode::isSuspended() const
{
	return (node_ && node_->isSuspended());
}

bool VNode::isAborted() const
{
	return (node_ && node_->state() == NState::ABORTED && !node_->isSuspended());
}

bool VNode::isSubmitted() const
{
    return (node_ && node_->state() == NState::SUBMITTED);
}

QColor  VNode::stateColour() const
{
	return VNState::toColour(this);
}

QColor  VNode::realStateColour() const
{
	return VNState::toRealColour(this);
}

QColor  VNode::stateFontColour() const
{
	return VNState::toFontColour(this);
}

QColor  VNode::typeFontColour() const
{
    return VNState::toTypeColour(this);
}

LogServer_ptr VNode::logServer()
{
	LogServer_ptr lsv;

	if(!node_)
		return lsv;

	std::string logHost=findInheritedVariable("ECF_LOGHOST",true);
	std::string logPort=findInheritedVariable("ECF_LOGPORT");
	//if(logHost.empty())
	//{
	//	logHost=findInheritedVariable("LOGHOST",true);
	//	logPort=findInheritedVariable("LOGPORT");
	//}

	std::string micro=findInheritedVariable("ECF_MICRO");
	if(!logHost.empty() && !logPort.empty() &&
	  (micro.empty() || logHost.find(micro) ==  std::string::npos))
	{
		lsv=LogServer_ptr(new LogServer(logHost,logPort));
		return lsv;
	}

	return lsv;
}

bool VNode::logServer(std::string& host,std::string& port)
{
	if(!node_)
		return false;

	host=findInheritedVariable("ECF_LOGHOST",true);
	port=findInheritedVariable("ECF_LOGPORT");
	//if(logHost.empty())
	//{
	//	logHost=findInheritedVariable("LOGHOST",true);
	//	logPort=findInheritedVariable("LOGPORT");
	//}

	std::string micro=findInheritedVariable("ECF_MICRO");
	if(!host.empty() && !port.empty() &&
	  (micro.empty() || host.find(micro) ==  std::string::npos))
	{
		return true;
	}

	return false;
}

#if 0
bool VNode::isAncestor(const VNode* n)
{
	if(n == this)
		return true;

    VNode* nd=parent();
    while(nd)
    {
    	if(n == nd)
           return true;

    	nd=nd->parent();
    }
    return false;
}
#endif

std::vector<VNode*> VNode::ancestors(SortMode sortMode)
{
	std::vector<VNode*> nodes;

	VNode* n=this;

	nodes.push_back(n);
	n=n->parent();

	if(sortMode == ChildToParentSort)
	{
		while(n)
		{
			nodes.push_back(n);
			n=n->parent();
		}
	}

	else if (sortMode == ParentToChildSort)
	{
		while(n)
		{
			nodes.insert(nodes.begin(),n);
			n=n->parent();
		}
	}

	return nodes;
}

VNode* VNode::ancestorAt(int idx,SortMode sortMode)
{
	if(sortMode == ChildToParentSort &&  idx==0)
		return this;

	std::vector<VNode*> nodes=ancestors(sortMode);

    if(nodes.size() > idx)
	{
		return nodes.at(idx);
	}

	return NULL;
}

const std::string&  VNode::nodeType()
{
	static std::string suiteStr("suite");
	static std::string familyStr("family");
	static std::string taskStr("task");
	static std::string defaultStr("node");
	static std::string serverStr("server");

	if(isServer())
		return serverStr;

	node_ptr np=node();

	if(!np || !np.get())
		return defaultStr;

	if(np->isSuite())
		return suiteStr;
	else if(np->isFamily())
		return familyStr;
	else if(np->isTask())
        return taskStr;

	return defaultStr;
}

#if 0
bool VNode::isFamily() const
{
    node_ptr np=node();

    if(!np || !np.get())
        return false;

    return (np->isFamily())?true:false;
}

bool VNode::isAlias() const
{
    node_ptr np=node();

    if(!np || !np.get())
        return false;

    return (np->isAlias())?true:false;
}
#endif

bool VNode::isFlagSet(ecf::Flag::Type f) const
{
	if(node_ && node_.get())
	{
		return node_->flag().is_set(f);
	}
	return false;
}

void VNode::why(std::vector<std::string>& theReasonWhy) const
{
    if(node_)
	{
		node_->bottom_up_why(theReasonWhy);
	}
}

const std::string& VNode::abortedReason() const
{
    if(node_)
	{
		return node_->abortedReason();
	}

	static std::string emptyStr;
	return emptyStr;

}

int VNode::labelNum() const
{
    return (!node_)?node_->labels().size():0;
}

int VNode::labelLineNum(int row) const
{
    if(!node_)
        return 1;

    const std::vector<Label>&  v=node_->labels();
    if(row >=0 && row < v.size())
    {
        std::string val=v[row].new_value();
        if(val.empty() || val == " ")
        {
            val=v[row].value();
        }
        return std::count(val.begin(), val.end(), '\n')+1;
    }

    return 1;
}

void VNode::statusChangeTime(QString& sct) const
{
    if(node_)
    {
        boost::posix_time::ptime t = node_->state_change_time();
        std::string s=boost::posix_time::to_simple_string(t);
        sct=QString::fromStdString(s);
    }
}

uint VNode::statusChangeTime() const
{
    if(node_)
    {
        static  boost::posix_time::ptime epoch(boost::gregorian::date(1970, 1, 1));
        boost::posix_time::ptime t = node_->state_change_time();
        boost::posix_time::time_duration diff(t - epoch);
        return diff.ticks()/diff.ticks_per_second();
    }
    return 0;
}

QString VNode::toolTip()
{    
    QString txt="<b>Name</b>: " + name() + "<br>";
    txt+="<b>Path</b>: " + QString::fromStdString(absNodePath()) + "<br>";
    txt+="<b>Type</b>: " + QString::fromStdString(nodeType()) + "<br>";
    
    txt+="<b>Status</b>: " + stateName();
    if(isSuspended())        
        txt+=" (" + VNState::toRealStateName(this) + ")";

    txt+="<br>";
    txt+="<b>Default status</b>: " + defaultStateName() + "<br>";
    
    txt+="<b>Server:</b> " + QString::fromStdString(server()->name()) + "<br>";
    txt+="<b>Host</b>: " + QString::fromStdString(server()->host());
    txt+=" <b>Port</b>: " + QString::fromStdString(server()->port());
    
    QString rs=QString::fromStdString(abortedReason());
    if(!rs.isEmpty())
    	txt+="<br><b>Aborted reason</b>:" + rs;

    return txt;
}   

//===========================================================
// Triggers
//===========================================================

void VNode::triggerExpr(std::string& trigger, std::string& complete) const
{
    if(node_)
    {
        QList<VItemTmp_ptr> v;
        VAttributeType::items("trigger",this,v);
        Q_FOREACH(VItemTmp_ptr aItem,v)
        {
            VAttribute* a=aItem->attribute();
            assert(a);
            std::string t;
            a->value("trigger_type",t);
            if(t=="0")
                a->value("trigger_expression",trigger);
            else if(t=="1")
                a->value("trigger_expression",complete);
        }       
    }
}

//Collect the information about all the triggers triggering this node
void VNode::triggers(TriggerCollector* tlc)
{
    VItemTmp_ptr nullItem;
    //Check the node itself
    //if(tlr.self())
    {
        //find nodes, event, meters and variables triggering this node
        if(node_ && !node_->isSuite())
        {
            std::vector<VItemTmp_ptr> theVec;
            AstCollateVNodesVisitor astVisitor(theVec);

            //Collect the nodes from ast
            if(node_->completeAst())
                node_->completeAst()->accept(astVisitor);

            if(node_->triggerAst())
                node_->triggerAst()->accept(astVisitor);

            //Add the found items to the collector
            for(std::vector<VItemTmp_ptr>::iterator it = theVec.begin(); it != theVec.end(); ++it)
            {
#ifdef _UI_VNODE_DEBUG
                //if((*it)->parent() && (*it)->parent()->absNodePath() == "/e_41r2_peter/main")
                //    qDebug() << "trigger ast:" << (*it)->name() << *it;
#endif
                tlc->add(*it,nullItem, TriggerCollector::Normal);
            }
        }

        //Check other attributes

        //Limiters
        QList<VItemTmp_ptr> limiterLst;
        VAttributeType::items("limiter",this,limiterLst);
        Q_FOREACH(VItemTmp_ptr aItem, limiterLst)
        {
            VAttribute* a=aItem->attribute();
            assert(a);
            std::string v;
            if(a->value("limiter_path",v))
            {
                if(VItemTmp_ptr n = findLimit(v, a->strName()))
                {
#ifdef _UI_VNODE_DEBUG
                    //qDebug() << "trigger limit:" << n->name();
#endif                   
                    tlc->add(n,nullItem, TriggerCollector::Normal);
                }
            }
        }

        //Date
        QList<VItemTmp_ptr> dateLst;
        VAttributeType::items("date",this,dateLst);
        Q_FOREACH(VItemTmp_ptr a, dateLst)
        {            
            tlc->add(a,nullItem,TriggerCollector::Normal);
        }

        //Time
        QList<VItemTmp_ptr> timeLst;
        VAttributeType::items("time",this,timeLst);
        Q_FOREACH(VItemTmp_ptr a, timeLst)
        {
             tlc->add(a,nullItem,TriggerCollector::Normal);
        }

    }

    if(tlc->scanParents())
    {
        VNode* p=parent();
        while(p)
        {
            TriggerParentCollector tpc(p,tlc);
            p->triggers(&tpc);
            p = p->parent();
        }
    }

    if(tlc->scanKids())
    {
        triggersInChildren(this,this,tlc);
    }
}

//Collect the triggers triggering node n in the children of parent
void VNode::triggersInChildren(VNode *n,VNode* p,TriggerCollector* tlc)
{
    for(size_t i=0; i < p->children_.size(); i++)
    {
        TriggerChildCollector tcc(n,p->children_[i],tlc);
        p->children_[i]->triggers(&tcc);
        triggersInChildren(n,p->children_[i],tlc);
  }
}

//These are called during the scan for triggered nodes
void VNode::addTriggeredData(VItem* n)
{
    if(!data_)
        data_=new VNodeTriggerData;

    data_->add(n);
}

void VNode::addTriggeredData(VItem* triggered,VItem* trigger)
{
    if(!data_)
       data_=new VNodeTriggerData;

    assert(trigger->parent() == this);
    data_->add(triggered,trigger);
}

//Collect the information about all the nodes this node or its attributes trigger
void VNode::triggered(TriggerCollector* tlc,TriggeredScanner* scanner)
{
    if(scanner && !root()->triggeredScanned())
    {
        unsigned int aNum=VAttribute::totalNum();
        scanner->start(root());
        root()->setTriggeredScanned(true);
        assert(aNum == VAttribute::totalNum());
    }

    //Get the nodes directly triggered by this node
    if(data_)
        data_->get(this,tlc);

    if(tlc->scanParents())
    {
        VNode* p=parent();
        while(p)
        {
            TriggerParentCollector tpc(p,tlc);
            p->triggered(&tpc);
            p = p->parent();
        }
    }

    if(tlc->scanKids())
    {
        triggeredByChildren(this,this,tlc);
    }
}

void VNode::triggeredByChildren(VNode *n,VNode* p,TriggerCollector* tlc)
{
    for(size_t i=0; i < p->children_.size(); i++)
    {
        TriggerChildCollector tcc(n,p->children_[i],tlc);
        p->children_[i]->triggered(&tcc);
        triggeredByChildren(n,p->children_[i],tlc);
    }
}

VItemTmp_ptr VNode::findLimit(const std::string& path, const std::string& name)
{
   // if (!strncmp("/", path.c_str(), 1))
   VItemTmp_ptr nullItem;

#if 0
   if(!path.empty() && path[0] == '/')
      if (! (f = serv().top()->find(path)))
         return &dummy_node::get(path + ":" + name);
#endif

    //If
    VNode* n=this;
    if(!path.empty() && path[0] == '/')
    {
        n=server()->vRoot()->find(path);
        if(!n)
            return nullItem;

        //if(n && n != this)
        //    return n->findLimit(path,name);
    }

    //Find the matching limit in the node
    QList<VItemTmp_ptr> limit;
    VAttributeType::items("limit",n,limit);
    Q_FOREACH(VItemTmp_ptr aItem, limit)
    {
       assert(aItem);
       assert(aItem->item());
       assert(aItem->item()->isAttribute());
       if(aItem->item()->strName() == name)
       {
           return aItem;
       }
    }

    //Find the matching limit in the ancestors
    VNode* p=n->parent();
    Q_ASSERT(p);
    for(int i=0; i < p->numOfChildren(); i++)
    {
        VNode* ch=p->childAt(i);
        if (ch->strName() == path.substr(0, ch->name().size()))
        {
            std::string::size_type next = path.find('/');
            return ch->findLimit(path.substr(next+1, path.size()), name);
        }
    }

    return nullItem;

   //return &dummy_node::get(path + ":" + name);
}


const std::string& VSuiteNode::typeName() const
{
   static std::string t("suite");
   return t;
}

const std::string& VFamilyNode::typeName() const
{
   static std::string t("family");
   return t;
}

const std::string& VAliasNode::typeName() const
{
   static std::string t("alias");
   return t;
}

//=================================================
//
// VNodeRoot - this represents the server
//
//=================================================

VServer::VServer(ServerHandler* server) :
	VNode(0,node_ptr()),
	server_(server),
    totalNum_(0),
    triggeredScanned_(false)
{
}

VServer::~VServer()
{
	clear();
}


int VServer::totalNumOfTopLevel(VNode* n) const
{
	if(!n->isTopLevel())
		return -1;

	int idx=indexOfChild(n);
	if(idx != -1)
		return totalNumOfTopLevel(idx);

	return -1;
}

int VServer::totalNumOfTopLevel(int idx) const
{
	assert(totalNumInChild_.size() == children_.size());

	if(idx >=0 && idx < totalNumInChild_.size())
	{
		return totalNumInChild_.at(idx);
	}

	return -1;
}

int VServer::totalNumOfTopLevel(const std::string& name) const
{
    for(size_t i=0; i < children_.size(); i++)
    {
        if(name == children_[i]->strName())
            return totalNumOfTopLevel(i);
    }
    return -1;
}

//--------------------------------
// Clear
//--------------------------------

//Clear the whole contents
void VServer::clear()
{
	if(totalNum_==0)
		return;

	cache_.clear();
	prevNodeState_.clear();

	bool hasNotifications=server_->conf()->notificationsEnabled();

	//Delete the children nodes. It will recursively delete all the nodes. It also saves the prevNodeState!!
	for(std::vector<VNode*>::const_iterator it=children_.begin(); it != children_.end(); ++it)
	{
		deleteNode(*it,hasNotifications);
	}

	//Clear the children vector
	children_.clear();

	totalNumInChild_.clear();

	//A sanity check
	assert(totalNum_ == 0);

	//Deallocate the nodes vector
	nodes_=std::vector<VNode*>();

    triggeredScanned_=false;
}

//Clear the contents of a particular VNode
/*void VServer::clear(VNode* node)
{
	//Delete the children of node. It will recursively delete all the nodes
	//below this node
	for(unsigned int i=0; i < node->numOfChildren(); i++)
	{
		deleteNode(node->childAt(i));
	}

	//Clear the node contents
	node->clear();
}*/

//Delete a particular node
void VServer::deleteNode(VNode* node,bool hasNotifications)
{
	for(unsigned int i=0; i < node->numOfChildren(); i++)
	{
		deleteNode(node->childAt(i),hasNotifications);
	}

	//If there are notifications we need to save previous state
	if(hasNotifications)
	{
		if(node->node_->isTask())
		{
			VNodeInternalState st;
			node->internalState(st);
			prevNodeState_[node->absNodePath()]=st;
		}
	}

	delete node;
	totalNum_--;
}

//------------------------------------------
// Variables
//------------------------------------------

int VServer::variablesNum() const
{
	return cache_.vars_.size();
}

int VServer::genVariablesNum() const
{
	return cache_.genVars_.size();
}

void VServer::variables(std::vector<Variable>& vars) const
{
	vars.clear();
	vars=cache_.vars_;
}

void VServer::genVariables(std::vector<Variable>& vars) const
{
	vars.clear();
	vars=cache_.genVars_;
}

std::string VServer::genVariable(const std::string& key) const
{
	std::string val;

	for(std::vector<Variable>::const_iterator it=cache_.genVars_.begin(); it != cache_.genVars_.end(); ++it)
	{
	   if((*it).name() == key)
		   val=(*it).theValue();
	}

	return val;
}

//------------------------------------------
// Find
//------------------------------------------

VNode* VServer::toVNode(const Node* nc) const
{
	return static_cast<VNode*>(nc->graphic_ptr());
}

VNode* VServer::find(const std::string& fullPath)
{
	if(fullPath.empty())
		return NULL;

	if(fullPath == "/")
		return this;

	std::vector<std::string> pathVec;
	boost::split(pathVec,fullPath,boost::is_any_of("/"));

	if(pathVec.size() > 0 && pathVec.at(0).empty())
	{
		pathVec.erase(pathVec.begin());
	}

	return VNode::find(pathVec);
}

std::string VServer::findVariable(const std::string& key,bool substitute) const
{
	std::string val;

	//Search user variables first
	for(std::vector<Variable>::const_iterator it=cache_.vars_.begin(); it != cache_.vars_.end(); ++it)
	{
		if((*it).name() == key)
		{
			val=(*it).theValue();
			if(substitute)
				val=substituteVariableValue(val);

			return val;
		}
	}

	//Then search server variables
	for(std::vector<Variable>::const_iterator it=cache_.genVars_.begin(); it != cache_.genVars_.end(); ++it)
	{
		if((*it).name() == key)
		{
			val=(*it).theValue();
			if(substitute)
				val=substituteVariableValue(val);

			return val;
		}
	}

	return val;
}

std::string VServer::findInheritedVariable(const std::string& key,bool substitute) const
{
	return findVariable(key,substitute);
}

std::string VServer::substituteVariableValue(const std::string& inVal) const
{
	std::string val=inVal;

	if(val.empty())
		return val;

	ServerDefsAccess defsAccess(server_);  // will reliquish its resources on destruction
	defs_ptr defs = defsAccess.defs();
	if (!defs)
		return val;

	defs->server().variableSubsitution(val);

	return val;
}

//----------------------------------------------
// Scan
//----------------------------------------------

//Clear the contents and get the number of children (suites)
//the server contain. At this point we do not build the tree.
void VServer::beginScan(VServerChange& change)
{
	//Clear the contents
	clear();
#if 0
	//Get the Defs.
	{
		ServerDefsAccess defsAccess(server_);  // will reliquish its resources on destruction
		defs_ptr defs = defsAccess.defs();
		if (!defs)
			return;

		const std::vector<suite_ptr> &suites = defs->suiteVec();
		change.suiteNum_=suites.size();

		std::vector<node_ptr> nv;
		defs->get_all_nodes(nv);
		change.totalNum_=change.suiteNum_+nv.size();

		//We need to update the cache server variables
		updateCache(defs);
	}

	//This will use ServerDefsAccess as well. So we have to be sure that t=the mutex is
	//released at this point.
	change.attrNum_=currentAttrNum();
#endif
    //Get the Defs.
    {
        ServerDefsAccess defsAccess(server_);  // will reliquish its resources on destruction
        defs_ptr defs = defsAccess.defs();
        if (!defs)
            return;

        //We need to update the cache server variables
        updateCache(defs);
    }
}

//Build the whole tree.
void VServer::endScan()
{
	totalNum_=0;

	//Get the Defs
	{
		ServerDefsAccess defsAccess(server_);  // will reliquish its resources on destruction
		defs_ptr defs = defsAccess.defs();
		if (!defs)
			return;

		bool hasNotifications=server_->conf()->notificationsEnabled();

		//Scan the suits.This will recursively scan all nodes in the tree.
		const std::vector<suite_ptr> &suites = defs->suiteVec();

		for(unsigned int i=0; i < suites.size();i++)
		{
            VNode* vn=new VSuiteNode(this,suites.at(i));
			totalNum_++;
			scan(vn,hasNotifications);
		}
	}

#if 0
	//This will use ServerDefsAccess as well. So we have to be sure that the mutex is
	//released at this point.
	endUpdateAttrNum();
#endif

	if(totalNum_ > 0)
	{
		nodes_.reserve(totalNum_);
		collect(nodes_);
		for(size_t i=0; i < nodes_.size(); i++)
			nodes_[i]->setIndex(i);
	}
}

void VServer::scan(VNode *node,bool hasNotifications)
{
	int prevTotalNum=totalNum_;

	std::vector<node_ptr> nodes;
	node->node()->immediateChildren(nodes);

	//totalNum_+=nodes.size();

    //Preallocates the children vector to the reqiuired size to save memory.
    if(nodes.size() > 0)
        node->children_.reserve(nodes.size());

	for(std::vector<node_ptr>::const_iterator it=nodes.begin(); it != nodes.end(); ++it)
	{
		VNode* vn=NULL;
		if((*it)->isTask())
		{
			vn=new VTaskNode(node,*it);

			//If there are notifications we need to check them using the previous state
			if(hasNotifications)
			{
				std::string path=(*it)->absNodePath();
				std::map<std::string,VNodeInternalState>::const_iterator itP=prevNodeState_.find(path);
				if(itP != prevNodeState_.end())
					vn->check(server_->conf(),itP->second);
            }
		}
        else if((*it)->isFamily())
		{
            vn=new VFamilyNode(node,*it);
		}
        else if((*it)->isAlias())
        {
            vn=new VAliasNode(node,*it);
        }
        else
        {
            assert(0);

        }
		totalNum_++;
		scan(vn,hasNotifications);
	}

	if(node->parent() == this)
	{
		 totalNumInChild_.push_back(totalNum_-prevTotalNum);
	}
}

VNode* VServer::nodeAt(int idx) const
{
	assert(idx>=0 && idx < nodes_.size());
	return nodes_.at(idx);
}

//----------------------------------------------
// Update
//----------------------------------------------

void VServer::beginUpdate(VNode* node,const std::vector<ecf::Aspect::Type>& aspect,VNodeChange& change)
{
	//NOTE: when this function is called the real node (Node) has already been updated. However the
	//views do not know about this change. So at this point (this is the begin step of the update)
	//all VNode functions have to return the values valid before the update happened!!!!!!!
	//The main goal of this function is to cleverly provide the views with some information about the nature of the update.

    //Update the generated variables. There is no notification about their change so we have to do it!!!
	if(node->node())
	{
		Suite *s=NULL;
		s=node->node()->isSuite();
		if(!s)
		{
			s=node->node()->suite();
		}

		if(s && s->begun())
		{
			node->node()->update_generated_variables();
		}

		if(node->nodeType() == "task")
		{
			bool stateCh=(std::find(aspect.begin(),aspect.end(),ecf::Aspect::STATE) != aspect.end());
			node->check(server_->conf(),stateCh);
		}
	}
#if 0
	bool attrNumCh=(std::find(aspect.begin(),aspect.end(),ecf::Aspect::ADD_REMOVE_ATTR) != aspect.end());
#endif
    bool nodeNumCh=(std::find(aspect.begin(),aspect.end(),ecf::Aspect::ADD_REMOVE_NODE) != aspect.end());

    bool attrNumCh=(std::find(aspect.begin(),aspect.end(),ecf::Aspect::ADD_REMOVE_ATTR) != aspect.end());
    if(attrNumCh)
    {
       node->rescanAttr();
    }

	//----------------------------------------------------------------------
	// The number of attributes changed but the number of nodes did not
	//----------------------------------------------------------------------

#if 0
	if(attrNumCh && !nodeNumCh)
	{
        //The attributes were never used. None of the views have ever
		//wanted to display/access these attributes so far, so we can
		//just ignore this update!!
		if(!node->isAttrNumInitialised())
		{
			change.ignore_=true;
		}
		//Otherwise we just register the number of attributes before and after the update
		else
		{
			node->beginUpdateAttrNum();

			//This it the current number of attributes stored in the real Node. This call will not change the
			//the number of attributes (attrNum_ stored in the VNode!!!!)
			change.attrNum_=node->currentAttrNum();

			//this is the number of attributes before the update.
			change.cachedAttrNum_=node->cachedAttrNum();
		}

		return;
	}
#endif
	//---------------------------------------------------------------------------------
	// The number of nodes changed.
	//---------------------------------------------------------------------------------
    if(nodeNumCh)
	{
		change.rescan_=true;
	}

	//In any other cases it is just a simple update (value or status changed)
}

//-------------------------------------------------------------------------------------------
// Finishes the update. It has to be consistent with the changes registered in VNodeChange.
// If anything does not match we return false that will call reset!!!
//-------------------------------------------------------------------------------------------

void VServer::endUpdate(VNode* node,const std::vector<ecf::Aspect::Type>& aspect,const VNodeChange& change)
{
#if 0
    bool attrNumCh=(std::find(aspect.begin(),aspect.end(),ecf::Aspect::ADD_REMOVE_ATTR) != aspect.end());
	bool nodeNumCh=(std::find(aspect.begin(),aspect.end(),ecf::Aspect::ADD_REMOVE_NODE) != aspect.end());

	//--------------------------------------------------------------
	// The number of attributes changed but the number of nodes did not
	//-------------------------------------------------------------

	if(attrNumCh && ! nodeNumCh)
	{
		//This call updates the number of attributes stored in the VNode
		node->endUpdateAttrNum();
	}
#endif
}

void VServer::beginUpdate(const std::vector<ecf::Aspect::Type>& aspect)
{
	//We need to update the cached server variables
	if(std::find(aspect.begin(),aspect.end(),ecf::Aspect::SERVER_VARIABLE) != aspect.end() ||
	   std::find(aspect.begin(),aspect.end(),ecf::Aspect::FLAG) != aspect.end())
	{
		//This will use the defs!!!
		updateCache();
	}
}

void VServer::suites(std::vector<std::string>& sv)
{
	for(int i=0; i < numOfChildren(); i++)
	{
		sv.push_back(children_.at(i)->strName());
	}
}

std::string VServer::strName() const
{
	if(server_)
		return server_->name();

	return std::string();


	/*if(name_.empty())
	{
		if(server_)
			name_=server_->name();
	}
	return name_;*/
}

QString VServer::stateName()
{
	if(VSState::isRunningState(server_))
	{
		return VNState::toName(server_);
	}

	return VSState::toName(server_);
}

QString VServer::defaultStateName()
{
	return stateName();
}

QString VServer::serverStateName()
{
	return VSState::toName(server_);
}

bool VServer::isSuspended() const
{
	return false;
}

QColor  VServer::stateColour() const
{
	if(VSState::isRunningState(server_))
	{
		return VNState::toColour(server_);
	}

	return VSState::toColour(server_);
}

QColor  VServer::stateFontColour() const
{
	if(VSState::isRunningState(server_))
	{
		return VNState::toFontColour(server_);
	}

	return VSState::toFontColour(server_);
}

void VServer::why(std::vector<std::string>& theReasonWhy) const
{
	ServerDefsAccess defsAccess(server_);  // will reliquish its resources on destruction
	defs_ptr defs = defsAccess.defs();
	if (!defs)
		return;

	defs->why(theReasonWhy);
}



bool VServer::isFlagSet(ecf::Flag::Type f) const
{
	return cache_.flag_.is_set(f);
}

void VServer::updateCache()
{
	cache_.clear();

	ServerDefsAccess defsAccess(server_);  // will reliquish its resources on destruction
	defs_ptr defs = defsAccess.defs();
	if (!defs)
		return;

	updateCache(defs);
}

void VServer::updateCache(defs_ptr defs)
{
	cache_.vars_=defs->server().user_variables();
	cache_.genVars_=defs->server().server_variables();
	cache_.flag_=defs->flag();
}

const std::string& VServer::typeName() const
{
   static std::string t("server");
   return t;
}

QString VServer::toolTip()
{
	QString txt="<b>Server</b>: " + QString::fromStdString(server_->name()) + "<br>";
	txt+="<b>Host</b>: " + QString::fromStdString(server_->host());
	txt+=" <b>Port</b>: " + QString::fromStdString(server_->port()) + "<br>";

	ConnectState* st=server_->connectState();

	if(server_->activity() == ServerHandler::LoadActivity)
	{
		txt+="<b>Server is being loaded!</b><br>";
		//txt+="<b>Started</b>: " + VFileInfo::formatDateAgo(st->lastConnectTime()) + "<br>";
	}
	else
	{
		if(st->state() == ConnectState::Normal)
		{
			txt+="<b>Server status</b>: " + VSState::toName(server_) + "<br>";
			txt+="<b>Status</b>: " + VNState::toName(server_) + "<br>";
			txt+="<b>Total number of nodes</b>: " +  QString::number(totalNum_);
		}
		else if(st->state() == ConnectState::Lost)
		{
			QColor colErr(255,0,0);
			txt+="<b><font color=" + colErr.name() +">Failed to connect to server!</b><br>";
			txt+="<b>Last connection</b>: " + VFileInfo::formatDateAgo(st->lastConnectTime()) + "<br>";
			txt+="<b>Last failed attempt</b>: " + VFileInfo::formatDateAgo(st->lastLostTime()) + "<br>";
			if(!st->errorMessage().empty())
				txt+="<b>Error message</b>:<br>" + QString::fromStdString(st->shortErrorMessage());
		}
		else if(st->state() == ConnectState::Disconnected)
		{
			QColor colErr(255,0,0);
			txt+="<b><font color=" + colErr.name() +">Server is disconnected!</b><br>";
			txt+="<b>Disconnected</b>: " + VFileInfo::formatDateAgo(st->lastDisconnectTime()) + "<br>";
		}
	}
	return txt;
}
