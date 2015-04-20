//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "VNode.hpp"

#include "Node.hpp"
#include "Variable.hpp"

#include "ServerHandler.hpp"
#include "VAttribute.hpp"
#include "VNState.hpp"
#include "VSState.hpp"

//=================================================
// VNode
//=================================================

VNode::VNode(VNode* parent,Node* node) :
    node_(node),
    parent_(parent),
    attrNum_(-1),
    cachedAttrNum_(-1)
{
	if(parent_)
		parent_->addChild(this);

	if(node_)
		node_->set_graphic_ptr(this);
}

bool VNode::isTopLevel() const
{
	return (node_)?(node_->isSuite() != NULL):false;
}
//At the beginning of the update we get the current number of attributes
void VNode::beginUpdateAttrNum()
{
	attrNum_=VAttribute::totalNum(node_);
}

//At the end of the update we set the cached value to the current number of attributes
void VNode::endUpdateAttrNum()
{
	cachedAttrNum_=attrNum_;
}

short VNode::cachedAttrNum() const
{
	return cachedAttrNum_;
}

short VNode::attrNum() const
{
	//If if was not initialised we get its value
	if(attrNum_==-1)
	{
		attrNum_=VAttribute::totalNum(node_);

		if(cachedAttrNum_ == -1)
			cachedAttrNum_=attrNum_;
	}

	return attrNum_;
}

QStringList VNode::getAttributeData(int row,VAttribute** type)
{
	QStringList lst;
	VAttribute::getData(node_,row,type,lst);
	return lst;
}

void VNode::addChild(VNode* vn)
{
	children_.push_back(vn);
}

VNode* VNode::childAt(int index) const
{
	return (index>=0 && index < children_.size())?children_.at(index):0;
}

int VNode::indexOfChild(const VNode* vn) const
{
	for(unsigned int i=0; i < children_.size(); i++)
	{
		if(children_.at(i) == vn)
			return i;
	}

	return -1;
}

int VNode::indexOfChild(Node* n) const
{
	for(unsigned int i=0; i < children_.size(); i++)
	{
		if(children_.at(i)->node() == n)
			return i;
	}

	return -1;
}


void VNode::replaceChildren(const std::vector<VNode*>& newCh)
{
	children_=newCh;
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


std::string VNode::absNodePath() const
{
	return (node_)?node_->absNodePath():"";
}

QString VNode::name() const
{
	return (node_)?QString::fromStdString(node_->name()):QString();
}

QString VNode::stateName()
{
	return VNState::toName(node_);
}

QString VNode::defaultStateName()
{
	return VNState::toDefaultStateName(node_);
}

bool VNode::isSuspended() const
{
	return (node_ && node_->isSuspended());
}

QColor  VNode::stateColour() const
{
	return VNState::toColour(node_);
}

LogServer_ptr VNode::logServer()
{
	LogServer_ptr lsv;

	std::string logHost=findInheritedVariable("ECF_LOGHOST",true);
	std::string logPort=findInheritedVariable("ECF_LOGPORT");
	if(logHost.empty())
	{
		logHost=findInheritedVariable("LOGHOST",true);
		logPort=findInheritedVariable("LOGPORT");
	}

	std::string micro=findInheritedVariable("ECF_MICRO");
	if(!logHost.empty() && !logPort.empty() &&
	  (micro.empty() || logHost.find(micro) ==  std::string::npos))
	{
		lsv=LogServer_ptr(new LogServer(logHost,logPort));
		return lsv;
	}

	return lsv;
}


/*
//Get the triggers list for the Triggers view
void VNode::triggers(TriggerList& tlr)
{
	//Check the node itself
	if(tlr.self())
	{
		if(node_ && !node_->isSuite())
		{
			std::set<VNode*> theSet;
			std::set<VNode*>::iterator sit;
			AstCollateXNodesVisitor astVisitor(theSet);

			if(node_->completeAst())
				node_->completeAst()->accept(astVisitor);

			if(node_->triggerAst())
				node_->triggerAst()->accept(astVisitor);

			for (sit = theSet.begin(); sit != theSet.end(); ++sit)
				tlr.next_node( *(*sit), 0, trigger_lister::normal, *sit);
     }

	 //Go through the attributes
     for(std::vector<VNode*>::iterator it=children_.begin(); it!= children_.end(); it++)
     {
        int type = node_->type();
        {
           ecf_concrete_node<InLimit const> *c =
                    dynamic_cast<ecf_concrete_node<InLimit const>*> (n->__node__());
           InLimit const * i = c ? c->get() : 0;
           if (i) {
              node *xn = 0;
              if ((xn = find_limit(i->pathToNode(), i->name())))
                 tlr.next_node(*xn,0,trigger_lister::normal,xn);
           }
        }
        if (type == NODE_DATE || type == NODE_TIME)
           tlr.next_node(*n,0,trigger_lister::normal,n);
     }
    }
  }

*/


//=================================================
//
// VNodeRoot - this represents the server
//
//=================================================

VNodeRoot::VNodeRoot(ServerHandler* server) :
	VNode(0,0),
	server_(server),
	totalNum_(0)
{
}

VNodeRoot::~VNodeRoot()
{
	clear();
}

void VNodeRoot::clear()
{
	for(std::vector<VNode*>::const_iterator it=children_.begin(); it != children_.end(); it++)
	{
		deleteNode(*it);
	}
	//A sanity check
	assert(totalNum_ == 0);
}


std::string VNodeRoot::absNodePath() const
{
	return "/";
}

VNode* VNodeRoot::find(const Node* nc) const
{
	return static_cast<VNode*>(nc->graphic_ptr());
}


std::string VNodeRoot::findVariable(const std::string& key,bool substitute) const
{
	std::string val;

	ServerDefsAccess defsAccess(server_);  // will reliquish its resources on destruction
	defs_ptr defs = defsAccess.defs();
	if (!defs)
		return val;

	const Variable& var=defs->server().findVariable(key);
    if(!var.empty())
    {
    	val=var.theValue();
    	if(substitute)
    	{
    		//defs->server().variableSubsitution(val);
    	}
    }
    return val;
}


std::string VNodeRoot::findInheritedVariable(const std::string& key,bool substitute) const
{
	return findVariable(key,substitute);
}

//Clear the contents and rebuild the whole tree.
void VNodeRoot::scan()
{
	//Clear the contents
	clear();

	if(server_->connected())
	{
		//Get the Defs
		ServerDefsAccess defsAccess(server_);  // will reliquish its resources on destruction
		defs_ptr defs = defsAccess.defs();
		if (!defs)
			return;

		//Scan the suits.This will recursively scan all nodes in the tree.
		const std::vector<suite_ptr> &suites = defs->suiteVec();
		for(unsigned int i=0; i < suites.size();i++)
		{
			VNode* vn=new VNode(this,suites.at(i).get());
			totalNum_++;
			scan(vn);
		}
	}
}

void VNodeRoot::scan(VNode *parent)
{
	std::vector<node_ptr> nodes;
	parent->node()->immediateChildren(nodes);

	totalNum_+=nodes.size();

	for(std::vector<node_ptr>::const_iterator it=nodes.begin(); it != nodes.end(); it++)
	{
		VNode* vn=new VNode(parent,(*it).get());
		scan(vn);
	}
}

void VNodeRoot::deleteNode(VNode* node)
{
	for(unsigned int i=0; i < node->numOfChildren(); i++)
	{
		deleteNode(node->childAt(i));
	}

	delete node;
	totalNum_--;
}

void VNodeRoot::beginUpdate(VNode* node,const std::vector<ecf::Aspect::Type>& aspect,VNodeChange& change)
{
	bool attrNumCh=(std::find(aspect.begin(),aspect.end(),ecf::Aspect::ADD_REMOVE_ATTR) != aspect.end());
	bool nodeNumCh=(std::find(aspect.begin(),aspect.end(),ecf::Aspect::ADD_REMOVE_NODE) != aspect.end());

	//------------------------------------------------------------
	// Number of attributes changed
	//-----------------------------------------------------------
	if(attrNumCh)
	{
		node->beginUpdateAttrNum();
		change.attrNum_=node->attrNum();
		change.cachedAttrNum_=node->cachedAttrNum();
	}

	//-------------------------------------------------------------
	// Number of nodes changed
	//-------------------------------------------------------------
	if(nodeNumCh)
	{
		std::vector<node_ptr> nodes;
		node->node()->immediateChildren(nodes);

		change.cachedNodeNum_=node->numOfChildren();
		change.nodeNum_=static_cast<int>(nodes.size());

		//----------------------------------------------------------------
		// We handle the situation when only one node was added and the
		// order of the nodes did not changed. It is only interesting
		// the number of attributes did not changed!!!!
		//----------------------------------------------------------------

		if(!attrNumCh && nodes.size() == node->numOfChildren()+1)
		{
			int added=0;
			int same=0;
			int addedAt=-1;

			//We need to be sure that all the nodes were really kept
			for(unsigned int i=0; i < nodes.size(); i++)
			{
				if(node->childAt(i-added)->node() == nodes.at(i).get())
				{
					same++;
				}
				else
				{
					//it is really a new node
					if(node->indexOfChild(nodes.at(i).get()) == -1)
					{
						added++;
						addedAt=i;
					}
				}
			}

			//If there was only one node added and the order of the node was kept
			//we register it in the change object
			if(same == node->numOfChildren() && added == 1 && addedAt != -1)
			{
				change.nodeAddedAt_=addedAt;
			}
		}

		//---------------------------------------------------------------------
		// Now we update the nodes. We need to take into account the order
		// as well.
		//---------------------------------------------------------------------

		//Go through the current list of children and see what exists and what has to be created.
		//All these are collected into upChildren.
		std::vector<VNode*> upChildren;
		for(std::vector<node_ptr>::const_iterator it=nodes.begin(); it != nodes.end(); it++)
		{
			//If the node is already present as a VNode
			int idx=-1;
			if((idx=node->indexOfChild((*it).get())) != -1)
			{
				upChildren.push_back(node->childAt(idx));
			}
			//It is new node
			else
			{
				//Create a Vnode and add it to its parent
				VNode* vn=new VNode(node,(*it).get());

				//Scan the newly added vnode
				scan(vn);

				upChildren.push_back(vn);
			}
		}

		//Now we need to see what has been deleted. All the VNodes with a deleted node
		//are collected into rmChildren.
		std::vector<VNode*> rmChildren;
		for(unsigned int i=0; i < node->numOfChildren(); i++)
		{
			if(std::find(upChildren.begin(),upChildren.end(),node->childAt(i)) ==  upChildren.end())
			{
				rmChildren.push_back(node->childAt(i));
			}
		}

		//We reset the list of children in the VNode!!
		node->replaceChildren(upChildren);

		//We delete the unused VNodes
		for(unsigned int i=0; i < rmChildren.size(); i++)
		{
			deleteNode(rmChildren[i]);
		}
	}
}

void VNodeRoot::endUpdate(VNode* node,const std::vector<ecf::Aspect::Type>& aspect)
{
	bool attrNumCh=(std::find(aspect.begin(),aspect.end(),ecf::Aspect::ADD_REMOVE_ATTR) != aspect.end());
	if(attrNumCh)
		node->endUpdateAttrNum();
}

LogServer_ptr VNodeRoot::logServer()
{
	LogServer_ptr lsv;
	return lsv;
}

QString VNodeRoot::stateName()
{
	if(VSState::isRunningState(server_))
	{
		return VNState::toName(server_);
	}

	return VNState::toName(server_);
}

QString VNodeRoot::defaultStateName()
{
	return stateName();
}

bool VNodeRoot::isSuspended() const
{
	return false;
}

QColor  VNodeRoot::stateColour() const
{
	if(VSState::isRunningState(server_))
	{
		return VNState::toColour(server_);
	}

	return VSState::toColour(server_);
}

