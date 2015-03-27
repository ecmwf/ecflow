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
#include "ServerHandler.hpp"
#include "VAttribute.hpp"

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

bool VNode::isTopLevel()
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

int VNode::indexOfChild(VNode* vn) const
{
	for(unsigned int i=0; i < children_.size(); i++)
	{
		if(children_.at(i) == vn)
			return i;
	}

	return -1;
}

void VNode::replaceChildren(const std::vector<VNode*>& newCh)
{
	children_=newCh;
}

//=================================================
// VNodeRoot
//=================================================

VNodeRoot::VNodeRoot(ServerHandler* server) : VNode(0,0), totalNum_(0)
{
	ServerDefsAccess defsAccess(server);  // will reliquish its resources on destruction
	defs_ptr defs = defsAccess.defs();

	const std::vector<suite_ptr> &suites = defs->suiteVec();
	for(unsigned int i=0; i < suites.size();i++)
	{
		VNode* vn=new VNode(this,suites.at(i).get());
		totalNum_++;
		scan(vn);
	}
}

VNodeRoot::~VNodeRoot()
{
	for(std::vector<VNode*>::const_iterator it=children_.begin(); it != children_.end(); it++)
	{
		deleteNode(*it);
	}

	//A sanity check
	assert(totalNum_ == 0);
}

VNode* VNodeRoot::find(const Node* nc) const
{
	return static_cast<VNode*>(nc->graphic_ptr());
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

void VNodeRoot::beginUpdate(VNode* node,const std::vector<ecf::Aspect::Type>& aspect) //,VNodeChange* change)
{
	bool attrNumCh=(std::find(aspect.begin(),aspect.end(),ecf::Aspect::ADD_REMOVE_ATTR) != aspect.end());
	bool nodeNumCh=(std::find(aspect.begin(),aspect.end(),ecf::Aspect::ADD_REMOVE_NODE) != aspect.end());

	//------------------------------------------------------------
	// Number of attributes changed but number of nodes did not
	//-----------------------------------------------------------
	if(attrNumCh && !nodeNumCh)
	{
		node->beginUpdateAttrNum();
	}

	//-------------------------------------------------------------
	// Number of nodes changed but number of attributes did not
	//-------------------------------------------------------------
	else if(!attrNumCh && nodeNumCh)
	{
		std::vector<node_ptr> nodes;
		node->node()->immediateChildren(nodes);

		//Go through the current list of children and see what existing and has to be created.
		//All these VNodes are collected into upChildren.
		std::vector<VNode*> upChildren;
		for(std::vector<node_ptr>::const_iterator it=nodes.begin(); it != nodes.end(); it++)
		{
			bool has=false;
			for(unsigned int i=0; i < node->numOfChildren(); i++)
			{
				if(node->childAt(i)->node() == (*it).get())
				{
					upChildren.push_back(node->childAt(i));
					break;
				}
			}
			if(!has)
			{
				VNode* vn=new VNode(node,(*it).get());
				scan(vn);
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

	//-------------------------------------------------------------
	// But the number of nodes and number of attributes changed
	//-------------------------------------------------------------
	else if(attrNumCh && nodeNumCh)
	{

	}

}

void VNodeRoot::endUpdate(VNode* node,const std::vector<ecf::Aspect::Type>& aspect)
{
	bool attrNumCh=(std::find(aspect.begin(),aspect.end(),ecf::Aspect::ADD_REMOVE_ATTR) != aspect.end());
	if(attrNumCh)
		node->endUpdateAttrNum();
}



//void* graphic_ptr() const { return  graphic_ptr_;}
