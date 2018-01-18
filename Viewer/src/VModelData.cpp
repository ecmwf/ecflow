//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "VModelData.hpp"

#include "AbstractNodeModel.hpp"
#include "ExpandState.hpp"
#include "NodeQuery.hpp"
#include "VFilter.hpp"
#include "ServerHandler.hpp"
#include "VAttribute.hpp"
#include "VAttributeType.hpp"
#include "VNode.hpp"
#include "VTree.hpp"
#include "UiLog.hpp"
#include "UIDebug.hpp"

#include <QDebug>
#include <QMetaMethod>

#define _UI_VMODELDATA_DEBUG

void VTreeChangeInfo::addStateChange(const VNode* n)
{
    VNode* s=n->suite();
    Q_ASSERT(s->isTopLevel());
    Q_ASSERT(s);
    if(std::find(stateSuites_.begin(),stateSuites_.end(),s) == stateSuites_.end())
        stateSuites_.push_back(s);
}

//==========================================
//
// VModelServer
//
//==========================================

//It takes ownership of the filter

VModelServer::VModelServer(ServerHandler *server) :
   server_(server),
   inScan_(0)
{
	//We has to observe the nodes of the server.
	server_->addNodeObserver(this);

    //We has to observe the server.
	server_->addServerObserver(this);

}

VModelServer::~VModelServer()
{
	server_->removeNodeObserver(this);
	server_->removeServerObserver(this);
}

int VModelServer::totalNodeNum() const
{
	return server_->vRoot()->totalNum();
}

//==========================================
//
// VTreeServer
//
//==========================================

//It takes ownership of the filter

VTreeServer::VTreeServer(ServerHandler *server,NodeFilterDef* filterDef,AttributeFilter* attrFilter) :
   VModelServer(server),
   changeInfo_(new VTreeChangeInfo()),
   attrFilter_(attrFilter),
   firstScan_(true),
   firstScanTryNo_(0),
   maxFirstScanTry_(10),
   expandState_(0)
{
    tree_=new VTree(this);
    filter_=new TreeNodeFilter(filterDef,server_,tree_);
	//We has to observe the nodes of the server.
	//server_->addNodeObserver(this);
}

VTreeServer::~VTreeServer()
{
    delete tree_;
    delete changeInfo_;
    delete filter_;
    if(expandState_)
        delete expandState_;
}

NodeFilter* VTreeServer::filter() const
{
    return const_cast<TreeNodeFilter*>(filter_);
}

int VTreeServer::nodeNum() const
{
    return tree_->totalNum();
}

void VTreeServer::adjustFirstScan()
{
    if(firstScan_)
    {
        if(nodeNum() > 0)
        {
            firstScan_=false;
        }
        else
        {
            firstScanTryNo_++;
            if(firstScanTryNo_ == maxFirstScanTry_)
            {
                firstScan_=false;
            }
        }
    }
}

//--------------------------------------------------
// ServerObserver methods
//--------------------------------------------------

void VTreeServer::notifyDefsChanged(ServerHandler* server, const std::vector<ecf::Aspect::Type>& a)
{
    //When the defs changed we need to update the server node in the model/view
    Q_EMIT dataChanged(this);
    //TODO: what about node or attr num changes!
}

void VTreeServer::notifyServerDelete(ServerHandler* s)
{
    UI_ASSERT(false, "server: " << s->longName());
    Q_ASSERT(0);
}

void VTreeServer::notifyBeginServerScan(ServerHandler* server,const VServerChange& change)
{
    //When the server scan begins we must be in inScan mode so that the model should think that
    //this server tree is empty.
    inScan_=true;
    UI_ASSERT(tree_->numOfChildren() == 0, "num: " << tree_->numOfChildren());
    changeInfo_->clear();
}

void VTreeServer::notifyEndServerScan(ServerHandler* /*server*/)
{
#ifdef _UI_VMODELDATA_DEBUG
    UI_FUNCTION_LOG_S(server_)
#endif
    UI_ASSERT(tree_->numOfChildren() == 0, "num: " << tree_->numOfChildren());

    //We still must be in inScan mode so that the model should think
    //that this server tree is empty.
    inScan_=true;

    filter_->clearForceShowNode();
    attrFilter_->clearForceShowAttr();

    //When the server scan ends we need to rebuild the tree.
    if(filter_->isComplete())
    {
        tree_->build();
    }
    else
    {
        filter_->update();
        tree_->build(filter_->match_);
    }

    int num=tree_->attrNum(attrFilter_)+tree_->numOfChildren();
    //Notifies the model of the number of children nodes to be added to the server node.
    Q_EMIT beginServerScan(this, num);
    //We leave the inScan mode. From this moment on the model can see the whole tree in the server.
    inScan_=false;
    //Notifies the model that the scan finished. The model can now relayout its new contents.
    Q_EMIT endServerScan(this, num);

    adjustFirstScan();
}

void VTreeServer::notifyBeginServerClear(ServerHandler* server)
{
    Q_EMIT beginServerClear(this,-1);
    changeInfo_->clear();
    tree_->clear();
    filter_->clear();
    attrFilter_->clearForceShowAttr();
    inScan_=true;
}

void VTreeServer::notifyEndServerClear(ServerHandler* server)
{
	Q_EMIT endServerClear(this,-1);
}

void VTreeServer::notifyServerConnectState(ServerHandler* server)
{
	Q_EMIT rerender();
}

void VTreeServer::notifyServerActivityChanged(ServerHandler* server)
{
	Q_EMIT dataChanged(this);
}

//This is called when a normal sync (neither reset nor rescan) is finished. We have delayed the update of the
//filter to this point but now we need to do it.
void VTreeServer::notifyEndServerSync(ServerHandler* server)
{
#ifdef _UI_VMODELDATA_DEBUG
    UI_FUNCTION_LOG_S(server_)
    UiLog(server_).dbg() << " number of state changes=" <<
                           changeInfo_->stateChangeSuites().size();
#endif

    updateFilter(changeInfo_->stateChangeSuites());
    changeInfo_->clear();
}

//--------------------------------------------------
// NodeObserver methods
//--------------------------------------------------

void VTreeServer::notifyBeginNodeChange(const VNode* vnode, const std::vector<ecf::Aspect::Type>& aspect, const VNodeChange& change)
{
#ifdef _UI_VMODELDATA_DEBUG
    UI_FUNCTION_LOG_S(server_)
#endif
    if(vnode==NULL)
		return;

    VTreeNode* node=tree_->find(vnode);

	bool attrNumCh=(std::find(aspect.begin(),aspect.end(),ecf::Aspect::ADD_REMOVE_ATTR) != aspect.end());
	bool nodeNumCh=(std::find(aspect.begin(),aspect.end(),ecf::Aspect::ADD_REMOVE_NODE) != aspect.end());

#ifdef _UI_VMODELDATA_DEBUG
    UiLog(server_).dbg() << " node=" << vnode->strName();
#endif

	//-----------------------------------------------------------------------
	// The number of attributes changed but the number of nodes is the same!
	//-----------------------------------------------------------------------
    if(node && attrNumCh && !nodeNumCh)
	{
        //We do not deal with the attributes if they were never used for the given node.
        //The first access to the attributes makes them initialised in the tree node.
        if(node->isAttrInitialised())
        {
            //Update the forceshow attribute in the filter because
            //it might have been deleted/reallocated
            attrFilter_->updateForceShowAttr();

            //This is the already updated attribute num
            int currentNum=vnode->attrNum(attrFilter_);

            //That is the attribute num we store in the tree node.
            int cachedNum=node->attrNum(attrFilter_);

            Q_ASSERT(cachedNum >= 0);

            int diff=currentNum-cachedNum;
            if(diff != 0)
            {
                Q_EMIT beginAddRemoveAttributes(this,node,currentNum,cachedNum);

                //We update the attribute num in the tree node
                node->updateAttrNum(attrFilter_);
                Q_EMIT endAddRemoveAttributes(this,node,currentNum,cachedNum);
            }

            //This can happen. When we change a trigger expression the change aspect we receive is ADD_REMOVE_ATTR.
            //In this case we just update all the attributes of the node!
            else
            {
                if(node)
                {
                    Q_EMIT attributesChanged(this,node);
                }
            }
        }
	}

	//----------------------------------------------------------------------
	// The number of nodes changed but number of attributes is the same!
	//----------------------------------------------------------------------
	else if(!attrNumCh && nodeNumCh)
	{
		//This can never happen
		assert(0);
	}

	//---------------------------------------------------------------------
	// Both the number of nodes and the number of attributes changed!
	//---------------------------------------------------------------------
	else if(attrNumCh && nodeNumCh)
	{
		//This can never happen
		assert(0);
	}

	//---------------------------------------------------------------------
	// The number of attributes and nodes did not change
	//---------------------------------------------------------------------
	else
	{
		//Check the aspects
		for(std::vector<ecf::Aspect::Type>::const_iterator it=aspect.begin(); it != aspect.end(); ++it)
		{
			//Changes in the nodes
			if(*it == ecf::Aspect::STATE || *it == ecf::Aspect::DEFSTATUS ||
			   *it == ecf::Aspect::SUSPENDED)
			{
                if(node && node->isAttrInitialised())
                {
                    Q_EMIT nodeChanged(this,node);
                }

                if(!vnode->isServer())
                    changeInfo_->addStateChange(vnode);

#ifdef _UI_VMODELDATA_DEBUG
                UiLog(server_).dbg() << " node status changed";
#endif               
			}

			//Changes might affect the icons
			else if (*it == ecf::Aspect::FLAG || *it == ecf::Aspect::SUBMITTABLE ||
					*it == ecf::Aspect::TODAY || *it == ecf::Aspect::TIME ||
					*it == ecf::Aspect::DAY || *it == ecf::Aspect::CRON || *it == ecf::Aspect::DATE)
			{
                if(node && node->isAttrInitialised())
                {
                    Q_EMIT nodeChanged(this,node);
                }
			}

			//Changes in the attributes
			if(*it == ecf::Aspect::METER ||  *it == ecf::Aspect::EVENT ||
			   *it == ecf::Aspect::LABEL || *it == ecf::Aspect::LIMIT ||
			   *it == ecf::Aspect::EXPR_TRIGGER || *it == ecf::Aspect::EXPR_COMPLETE ||
			   *it == ecf::Aspect::REPEAT || *it == ecf::Aspect::NODE_VARIABLE ||
			   *it == ecf::Aspect::LATE || *it == ecf::Aspect::TODAY || *it == ecf::Aspect::TIME ||
			   *it == ecf::Aspect::DAY || *it == ecf::Aspect::CRON || *it == ecf::Aspect::DATE )
			{
                if(node && node->isAttrInitialised())
                {
                    Q_EMIT attributesChanged(this,node);
                }
			}
		}
	}
}

void VTreeServer::notifyEndNodeChange(const VNode* vnode, const std::vector<ecf::Aspect::Type>& aspect, const VNodeChange& change)
{
}

void VTreeServer::reload()
{
#ifdef _UI_VMODELDATA_DEBUG
    UI_FUNCTION_LOG
#endif

    changeInfo_->clear();

    Q_EMIT beginServerClear(this,-1);
    tree_->clear();
    inScan_=true;
    Q_EMIT endServerClear(this,-1);

    Q_ASSERT(filter_);

    filter_->clearForceShowNode();
    if(filter_->isComplete())
    {
        tree_->build();
    }
    else
    {
        filter_->update();
        tree_->build(filter_->match_);
    }

    Q_EMIT beginServerScan(this, tree_->attrNum(attrFilter_)+tree_->numOfChildren());
    inScan_=false;
    Q_EMIT endServerScan(this, tree_->attrNum(attrFilter_)+tree_->numOfChildren());

    adjustFirstScan();
}

void VTreeServer::attrFilterChanged()
{   
    //In the tree root the attrNum must be cached/initialised
    Q_ASSERT(tree_->isAttrInitialised());
    int oriNum=tree_->attrNum(attrFilter_)+tree_->numOfChildren();
    Q_EMIT beginServerClear(this,oriNum);
    inScan_=true;
    Q_EMIT endServerClear(this,oriNum);

    //We reset the attrNum to the uninitailsed value in the whole tree
    tree_->resetAttrNum();

    //Get the current num of attr and children
    int num=tree_->attrNum(attrFilter_)+tree_->numOfChildren();
    Q_EMIT beginServerScan(this,num);
    inScan_=false;
    Q_EMIT endServerScan(this,num);
}


//This is called when a normal sync (neither reset nor rescan) is finished. We have delayed the update of the
//filter to this point but now we need to do it.
//It is also called when a forceShowNode is set.
//
//The vector suitesChanged contains all the suites in which a change happened. The filter
//will only be updated for the branches of these nodes.
void VTreeServer::updateFilter(const std::vector<VNode*>& suitesChanged)
{
#ifdef _UI_VMODELDATA_DEBUG
    UI_FUNCTION_LOG_S(server_)
#endif

    //if there was a state change during the sync
    if(suitesChanged.size() > 0 && !filter_->isNull() && !filter_->isComplete())
    {
#ifdef _UI_VMODELDATA_DEBUG      
        if(suitesChanged.size() < 0)
            UiLog(server_).dbg() << " suites changed:";
        for(size_t i= 0; i < suitesChanged.size(); i++)
            UiLog(server_).dbg() << "  " << suitesChanged[i]->strName();
#endif
        //Update the filter for the suites with a status change. topFilterChange
        //will contain the branches where the filter changed. A branch is
        //defined by the top level node with a filter status change in a given
        //suite. A branch cannot be a server (root) but at most a suite.
        std::vector<VNode*> topFilterChange;
        filter_->update(suitesChanged,topFilterChange);

#ifdef _UI_VMODELDATA_DEBUG
        if(topFilterChange.size() > 0)
            UiLog(server_).dbg() << " top level nodes that changed in filter:";
        for(size_t i= 0; i < topFilterChange.size(); i++)
            UiLog(server_).dbg() << "  " << topFilterChange.at(i)->strName();
#endif

        //A topFilterChange branch cannot be the root (server)
        for(size_t i=0; i < topFilterChange.size(); i++)
        {
            Q_ASSERT(!topFilterChange[i]->isServer());
        }
#ifdef _UI_VMODELDATA_DEBUG
        UiLog(server_).dbg() << " apply changes to branches";
#endif
        //If something changed in the list of filtered nodes
        for(size_t i=0; i < topFilterChange.size(); i++)
        {
#ifdef _UI_VMODELDATA_DEBUG
            UiLog(server_).dbg() << "  branch: " << topFilterChange[i]->absNodePath();
#endif
            //If the filter status of a SUITE has changed
            if(topFilterChange[i]->isSuite())
            {
                VNode *suite=topFilterChange[i];
                Q_ASSERT(suite);

                //This is the branch where there is a change in the filter
                VTreeNode* tn=tree_->find(topFilterChange[i]);

                //Remove the suite if it is in the tree
                if(tn)
                {
                    Q_ASSERT(!tn->isRoot());

#ifdef _UI_VMODELDATA_DEBUG
                    UiLog(server_).dbg() << "  remove suite: " <<  suite->absNodePath();
#endif
                    int index=tree_->indexOfTopLevel(tn);
                    Q_ASSERT(index >=0);

                    Q_EMIT beginFilterUpdateRemoveTop(this,index);
                    tree_->remove(tn);
                    Q_EMIT endFilterUpdateRemoveTop(this,index);
                }
                //Add the suite if it is NOT in the tree
                else
                {
#ifdef _UI_VMODELDATA_DEBUG
                    UiLog(server_).dbg() << "  add suite: " << suite->absNodePath();
#endif
                    VTreeNode *branch=tree_->makeTopLevelBranch(filter_->match_,suite);
                    int index=tree_->indexOfTopLevelToInsert(suite);
                    Q_ASSERT(index >=0);

                    Q_EMIT beginFilterUpdateInsertTop(this,index);
                    tree_->insertTopLevelBranch(branch,index);
                    Q_EMIT endFilterUpdateInsertTop(this,index);
                }
            }

            //If the top level node that changed is not a suite
            else
            {
                //We need to find the nearest existing parent in the tree
                VTreeNode* tn=tree_->findAncestor(topFilterChange[i]);

                //This must be at most a suite. It cannot be the root!
                Q_ASSERT(tn);
                Q_ASSERT(!tn->isRoot());

#ifdef _UI_VMODELDATA_DEBUG
                 UiLog(server_).dbg() << "  tree node to update: "  << tn->vnode()->absNodePath();
#endif
                //First, we remove the branch contents
                if(tn->numOfChildren() >0)
                {
                    int totalRows=tn->attrNum(attrFilter_) + tn->numOfChildren();
                    Q_EMIT beginFilterUpdateRemove(this,tn,totalRows);
                    tree_->removeChildren(tn);
                    Q_EMIT endFilterUpdateRemove(this,tn,totalRows);
                }

                //Second, we add the new contents
                VTreeNode *branch=tree_->makeBranch(filter_->match_,tn);
                int chNum=branch->numOfChildren();

                Q_EMIT beginFilterUpdateAdd(this,tn,chNum);
                if(chNum > 0)
                {
                    tree_->replaceWithBranch(tn,branch);
                }
                Q_EMIT endFilterUpdateAdd(this,tn,chNum);

                //branch must be empty now
                Q_ASSERT(branch->numOfChildren() == 0);
                delete branch;

            }
        }
    }
}


//Set the forceShowNode and rerun the filter. The forceShowNode is a node that
//has to be visible even if it does not match the status filter. There can be at most one
//forceShowNode at any time.
void VTreeServer::setForceShowNode(const VNode* node)
{
#ifdef _UI_VMODELDATA_DEBUG
    UI_FUNCTION_LOG_S(server_)
#endif

    Q_ASSERT(node);

    if(node == filter_->forceShowNode())
        return;

    //There is no status filter
    if(filter_->isNull())
    {
        filter_->setForceShowNode(const_cast<VNode*>(node));
        return;
    }
    //We have a status filter. We need to rerun it for the 
    //branch of the forceShow node
    else
    {
        //find the suite of the node
        VNode* s=node->suite();
        Q_ASSERT(s);
        Q_ASSERT(s->isTopLevel());

        std::vector<VNode*> sv;
        sv.push_back(s);

        //modify the filter definition
        filter_->setForceShowNode(const_cast<VNode*>(node));

        //run the filter
        updateFilter(sv);
    }
}

void VTreeServer::setForceShowAttribute(const VAttribute* a)
{
#ifdef _UI_VMODELDATA_DEBUG
    UI_FUNCTION_LOG_S(server_)
#endif

    Q_ASSERT(a);
    VNode* vnode=a->parent();
    Q_ASSERT(vnode);

    VTreeNode* node=tree_->find(vnode);

#ifdef _UI_VMODELDATA_DEBUG
    UiLog(server_).dbg() << "  node=" << node << " Attr=" << a->name() << " type=" << a->typeName();
#endif

    //Clear
    clearForceShow(a);

#ifdef _UI_VMODELDATA_DEBUG
    UiLog(server_).dbg() << "  node after clear=" << node;
#endif

    //clearForce might have removed the node, so we need to find it again!
    node=tree_->find(vnode);

    //Tell the attribute filter that this attribute must always be visible
    //attrFilter_->setForceShowAttr(const_cast<VAttribute*>(a));

    //Tell the tree that this node must always be visible
    filter_->setForceShowNode(const_cast<VNode*>(vnode));

    //The node is not visible at the moment e.i. not in the tree. We rerun
    //updatefilter in the nodes's branch. This will add the node to the tree
    //and will result in displaying the right attributes as well.
    if(!node)
    {
#ifdef _UI_VMODELDATA_DEBUG
    UiLog(server_).dbg() << "  node does not exist -->" << node;
#endif

        //find the suite
        VNode* s=vnode->suite();
        Q_ASSERT(s->isTopLevel());
        Q_ASSERT(s);

        std::vector<VNode*> sv;
        sv.push_back(s);

        //Tell the attribute filter that this attribute must always be visible
        attrFilter_->setForceShowAttr(const_cast<VAttribute*>(a));

        updateFilter(sv);
    }
    //The attribute is not visible (its type is not filtered)
    else if(!attrFilter_->isSet(a->type()))
    {
#ifdef _UI_VMODELDATA_DEBUG
        UiLog(server_).dbg() << "  attribute type is not filtered -->";
#endif

        //We only need to handle this case. When the attributes are not yet initialised
        //the selection in the view will trigger the attribute initialisation. This
        //will use the filter that we already set to use the attribute (as forceShowAttr).
        if(node->isAttrInitialised())
        {
            //This is the attribute num we store in the tree node
            //(and display in the tree).
            int cachedNum=node->attrNum(attrFilter_);

            //Tell the attribute filter that this attribute must always be visible
            attrFilter_->setForceShowAttr(const_cast<VAttribute*>(a));

            //This is the current attribute num using the modified attribute filter
            int currentNum=vnode->attrNum(attrFilter_);

            //This is the attribute num we store in the tree node
            //(and display in the tree).
            //int cachedNum=node->attrNum(attrFilter_);
            Q_ASSERT(cachedNum >= 0);           

#ifdef _UI_VMODELDATA_DEBUG
            UiLog(server_).dbg() << "  currentNum=" << currentNum << " cachedNum=" << cachedNum;
#endif
            if(currentNum != cachedNum)
            {
                Q_EMIT beginAddRemoveAttributes(this,node,currentNum,cachedNum);

                //We update the attribute num in the tree node
                node->updateAttrNum(attrFilter_);

                //This will trigger rerendering the attributes
                Q_EMIT endAddRemoveAttributes(this,node,currentNum,cachedNum);
            }
            //This will trigger rerendering the attributes of the given node when
            //currentNum and cachedNum are the same.
            else
            {
                Q_EMIT attributesChanged(this,node);
            }
        }
        else
        {
            //Tell the attribute filter that this attribute must always be visible
            attrFilter_->setForceShowAttr(const_cast<VAttribute*>(a));
        }
    }
}

void VTreeServer::setForceShow(const VItem* item)
{
    if(!item)
        return;

    //Get the server
    if(item->server() == server_)
    {
        if(VNode *n=item->isNode())
        {
            setForceShowNode(n);
        }
        else if(VAttribute* a=item->isAttribute())
        {
            setForceShowAttribute(a);
        }
    }
}


//Clear the forceShow if it does not match itemNext, which is the the next item 
//to be set as forceShow
void VTreeServer::clearForceShow(const VItem* itemNext)
{
#ifdef _UI_VMODELDATA_DEBUG
    UI_FUNCTION_LOG_S(server_)
#endif

    if(!itemNext)
        return;

    //The stateFilter is unique for each VTreeServer while the AttrFilter is
    //shared by the servers!
    VNode* vnPrev=filter_->forceShowNode();
    VAttribute* aPrev=attrFilter_->forceShowAttr();

    //If there is a forceShowAttribute
    if(aPrev)
    {
        Q_ASSERT(aPrev->parent()->server());
        //The stored attribute belongs to this server
        if(aPrev->parent()->server()== server_)
        {
            Q_ASSERT(vnPrev);
            Q_ASSERT(aPrev->parent() == vnPrev);
        }
        //Otherwise we pretend it is 0
        else
            aPrev=0;
    }

    //No forceShowNode or forceShow attribute is set. There is nothing to clear!
    if(!vnPrev && !aPrev)
        return;

    //We need to figure out if itemNext is the same foreceShow that we
    //currently store because in this case there is nothing to do.

    //Get the server
    ServerHandler *sh=itemNext->server();

    //The server matches
    if(sh == server_)
    {
        //itemNext is a node and it is the same that we store
        if(VNode *itn=itemNext->isNode())
        {
            //The current forceShow is a node
            if(!aPrev)
            {
                //it is the same that we store or no state filter is defined
                if(itn == vnPrev || filter_->isNull())
                    return;
            }
        }
        //itemNext is an attribute
        if(VAttribute *ita=itemNext->isAttribute())
        {
            if(aPrev)
            {
                //The attribute is in the current branch and both the current and next 
                //attribute type is filtered
                if(ita->parent() == aPrev->parent() &&
                   attrFilter_->isSet(aPrev->type()) && attrFilter_->isSet(ita->type()))
                {
                    return;
                }
                //the attribute is the same as before
                if(aPrev->sameContents(ita))
                {
                    return;
                }
            }
            //The item is an attribute (child) of the current showForceNode
            else if(vnPrev && vnPrev == ita->parent())
                return;
        }
    }

    //Need to remove the current showForce attribute
    if(aPrev)
    {
        //Remove the current showForce attribute from the attribute filter
        attrFilter_->clearForceShowAttr();

        //Rebuild the branch of the current showForce node with the 
        //current attribute filter
        VTreeNode *node=tree_->find(vnPrev);
        if(node)
        {
            Q_ASSERT(node->isAttrInitialised());

            //This is the actual num with the current filter
            int currentNum=vnPrev->attrNum(attrFilter_);

            //This is the attribute num we store in the tree node
            //(and display in the tree).
            int cachedNum=node->attrNum(attrFilter_);
            Q_ASSERT(cachedNum >= 0);
            Q_ASSERT(currentNum <= cachedNum);

            Q_EMIT beginAddRemoveAttributes(this,node,currentNum,cachedNum);

            //Update the attribute num in the tree node
            node->updateAttrNum(attrFilter_);

            //This will trigger rerendering the attributes of the given node
            //even if currentNum and cachedNum are the same.
            Q_EMIT endAddRemoveAttributes(this,node,currentNum,cachedNum);
        }
    }

    //Remove the current showForce node from the node status filter
    filter_->clearForceShowNode();

    //Reload the node status filter
    VNode* s=vnPrev->suite();
    Q_ASSERT(s->isTopLevel());
    Q_ASSERT(s);

    std::vector<VNode*> sv;
    sv.push_back(s);

    updateFilter(sv);
}

void VTreeServer::deleteExpandState()
{
    if(expandState_)
        delete expandState_;
    expandState_=0;
}

void VTreeServer::setExpandState(ExpandState* es)
{
    if(expandState_)
        delete expandState_;

    expandState_=es;
}

//==========================================
//
// VTableServer
//
//==========================================

//It takes ownership of the filter

VTableServer::VTableServer(ServerHandler *server,NodeFilterDef* filterDef) :
    VModelServer(server)
{
    filter_=new TableNodeFilter(filterDef,server);

    //We have to observe the nodes of the server.
	//server_->addNodeObserver(this);
}

VTableServer::~VTableServer()
{
    delete filter_;
}

NodeFilter* VTableServer::filter() const
{
    return const_cast<TableNodeFilter*>(filter_);
}

int VTableServer::nodeNum() const
{
    return filter_->matchCount();
}

//Node at the position in the list of filtered nodes
VNode* VTableServer::nodeAt(int index) const
{
    return filter_->nodeAt(index);
}

//Position in the list of filtered nodes
int VTableServer::indexOf(const VNode* node) const
{
    return filter_->indexOf(node);
}

//--------------------------------------------------
// ServerObserver methods
//--------------------------------------------------

void VTableServer::notifyServerDelete(ServerHandler* s)
{
    UI_ASSERT(false,"server: " << s->longName());
}

void VTableServer::notifyBeginServerScan(ServerHandler* server,const VServerChange& change)
{
    inScan_=true;
    Q_ASSERT(nodeNum() == 0);
	//At this point we do not know how many nodes we will have in the filter!
}

void VTableServer::notifyEndServerScan(ServerHandler* server)
{
    Q_ASSERT(inScan_);
    filter_->update();
    Q_EMIT beginServerScan(this,nodeNum());
    inScan_=false;
    Q_EMIT endServerScan(this,nodeNum());
}

void VTableServer::notifyBeginServerClear(ServerHandler* server)
{
    Q_EMIT beginServerClear(this,nodeNum());
}

void VTableServer::notifyEndServerClear(ServerHandler* server)
{
    int oriNodeNum=nodeNum();
    filter_->clear();
    //filter_->clearForceShowNode();
    Q_EMIT endServerClear(this,oriNodeNum);
}

void VTableServer::notifyServerConnectState(ServerHandler* server)
{
	Q_EMIT rerender();
}

void VTableServer::notifyServerActivityChanged(ServerHandler* server)
{
	Q_EMIT dataChanged(this);
}

//This is called when a normal sync (no reset or rescan) is finished. We have delayed the update of the
//filter to this point but now we need to do it.
void VTableServer::notifyEndServerSync(ServerHandler*)
{
    reload();
}

void VTableServer::notifyBeginNodeChange(const VNode* node, const std::vector<ecf::Aspect::Type>& types,const VNodeChange&)
{
    Q_EMIT nodeChanged(this,node);
}

void VTableServer::notifyEndNodeChange(const VNode* node, const std::vector<ecf::Aspect::Type>& types,const VNodeChange&)
{
}

void VTableServer::reload()
{
#ifdef _UI_VMODELDATA_DEBUG
    UI_FUNCTION_LOG_S(server_)
#endif

    int oriNodeNum=nodeNum();
#ifdef _UI_VMODELDATA_DEBUG
    UiLog(server_).dbg() << " oriNodeNum=" << oriNodeNum;
#endif

    Q_EMIT beginServerClear(this,oriNodeNum);
    VNode *fsn=filter_->forceShowNode();
    filter_->clear();
    filter_->setForceShowNode(fsn);
    inScan_=true;
    Q_EMIT endServerClear(this,oriNodeNum);

    filter_->update();

    Q_EMIT beginServerScan(this, nodeNum());
    inScan_=false;
    Q_EMIT endServerScan(this, nodeNum());

#ifdef _UI_VMODELDATA_DEBUG
    UiLog(server_).dbg() << " nodeNum: " << oriNodeNum;
#endif
}

//Set the forceShowNode and rerun the filter. The forceShowNode is a node that
//has to be visible even if it does not match the filter.
void VTableServer::setForceShowNode(const VNode* node)
{
    if(inScan_)
        return;

    if(filter_->indexOf(node) != -1)
    {
        clearForceShow(node);
        return;
    }

    Q_ASSERT(node);

    filter_->setForceShowNode(const_cast<VNode*>(node));
    reload();
}

void VTableServer::setForceShowAttribute(const VAttribute*)
{
}

void VTableServer::clearForceShow(const VItem* item)
{
    if(!item)
        return;

    VNode* vnPrev=filter_->forceShowNode();
    if(!vnPrev)
        return;

    if(item->parent()->server() == server_)
    {
        if(VNode *itn=item->isNode())
        {
            if(itn == vnPrev)
                return;
        }

        if(VAttribute *ita=item->isAttribute())
        {
            if(ita->parent() == vnPrev)
            {
                return;
            }
        }
    }

    filter_->clearForceShowNode();
    reload();
}

//==========================================
//
// VModelData
//
//==========================================

VModelData::VModelData(NodeFilterDef *filterDef,AbstractNodeModel* model) :
		QObject(model),
        serverNum_(0),
        serverFilter_(0),
		filterDef_(filterDef),
        model_(model),
        active_(false)
{
	connect(filterDef_,SIGNAL(changed()),
			this,SLOT(slotFilterDefChanged()));

	connect(this,SIGNAL(filterDeleteBegin()),
			model_,SLOT(slotFilterDeleteBegin()));

	connect(this,SIGNAL(filterDeleteEnd()),
			model_,SLOT(slotFilterDeleteEnd()));

    connect(this,SIGNAL(serverAddBegin(int)),
			model_,SLOT(slotServerAddBegin(int)));

	connect(this,SIGNAL(serverAddEnd()),
			model_,SLOT(slotServerAddEnd()));

    connect(this,SIGNAL(serverRemoveBegin(VModelServer*,int)),
            model_,SLOT(slotServerRemoveBegin(VModelServer*,int)));

    connect(this,SIGNAL(serverRemoveEnd(int)),
            model_,SLOT(slotServerRemoveEnd(int)));

#if 0
    connect(this,SIGNAL(filterChangeBegun()),
            model_,SIGNAL(filterChangeBegun()));

    connect(this,SIGNAL(filterChangeEnded()),
           model_,SIGNAL(filterChangeEnded()));
#endif

}

VModelData::~VModelData()
{
	clear();
}

void VModelData::connectToModel(VModelServer* d)
{
    connect(d,SIGNAL(dataChanged(VModelServer*)),
        model_,SLOT(slotDataChanged(VModelServer*)));

	connect(d,SIGNAL(beginServerScan(VModelServer*,int)),
		model_,SLOT(slotBeginServerScan(VModelServer*,int)));

	connect(d,SIGNAL(endServerScan(VModelServer*,int)),
		model_,SLOT(slotEndServerScan(VModelServer*,int)));

	connect(d,SIGNAL(beginServerClear(VModelServer*,int)),
		model_,SLOT(slotBeginServerClear(VModelServer*,int)));

	connect(d,SIGNAL(endServerClear(VModelServer*,int)),
		model_,SLOT(slotEndServerClear(VModelServer*,int)));

	//The model relays this signal
	connect(d,SIGNAL(rerender()),
		model_,SIGNAL(rerender()));
}

//Completely clear the data and rebuild everything with a new
//ServerFilter.
void VModelData::reset(ServerFilter* serverFilter)
{
	clear();
	serverFilter_=serverFilter;
	init();
}

void VModelData::init()
{
	serverFilter_->addObserver(this);

	for(unsigned int i=0; i < serverFilter_->items().size(); i++)
	{
        if(ServerHandler *server=serverFilter_->items().at(i)->serverHandler())
		{
			add(server);
		}
	}
}

void VModelData::addToServers(VModelServer* s)
{
    servers_.push_back(s);
    serverNum_=servers_.size();
}

void VModelData::clear()
{
#ifdef _UI_VMODELDATA_DEBUG
    UI_FUNCTION_LOG
#endif

    if(serverFilter_)
		serverFilter_->removeObserver(this);

    serverFilter_=NULL;

    for(std::size_t i=0; i < servers_.size(); i++)
	{
        delete servers_[i];
	}

	servers_.clear();
    serverNum_=0;
}

VModelServer* VModelData::server(int n) const
{
    return (n >=0 && n < static_cast<int>(servers_.size()))?servers_[n]:0;
}

ServerHandler* VModelData::serverHandler(int n) const
{
    return (n >=0 && n < static_cast<int>(servers_.size()))?servers_[n]->server_:0;
}

int VModelData::indexOfServer(void* idPointer) const
{
    for(std::size_t i=0; i < servers_.size(); i++)
        if(servers_[i] == idPointer)
			return i;

	return -1;
}

ServerHandler* VModelData::serverHandler(void* idPointer) const
{
    for(int i=0; i < serverNum_; i++)
        if(servers_[i] == idPointer)
            return servers_[i]->server_;

	return NULL;
}

VModelServer* VModelData::server(const void* idPointer) const
{
    for(int i=0; i < serverNum_; i++)
        if(servers_[i] == idPointer)
            return servers_[i];

	return NULL;
}

VModelServer* VModelData::server(const std::string& name) const
{
    for(int i=0; i < serverNum_; i++)
        if(servers_[i]->server_->name()  == name)
            return servers_[i];

    return NULL;
}

VModelServer* VModelData::server(ServerHandler* s) const
{
    for(int i=0; i < serverNum_; i++)
        if(servers_[i]->server_ == s)
            return servers_[i];

    return NULL;
}


int VModelData::indexOfServer(ServerHandler* s) const
{
    for(int i=0; i < serverNum_; i++)
        if(servers_[i]->server_ == s)
			return i;
	return -1;
}

int VModelData::numOfNodes(int index) const
{
	if(VModelServer *d=server(index))
	{
        return d->nodeNum();
	}
	return 0;
}

//ServerFilter observer methods

void VModelData::notifyServerFilterAdded(ServerItem* item)
{
	if(!item)
		return;

	if(ServerHandler *server=item->serverHandler())
	{
		//Notifies the model that a change will happen
		Q_EMIT serverAddBegin(count());

		add(server);

		//Notifies the model that the change has finished
		Q_EMIT serverAddEnd();
		return;
	}
}

void VModelData::notifyServerFilterRemoved(ServerItem* item)
{
#ifdef _UI_VMODELDATA_DEBUG
    UI_FUNCTION_LOG
#endif

    if(!item)
		return;

#ifdef _UI_VMODELDATA_DEBUG
    UiLog().dbg() << " server=" << item->longName();
#endif

	int i=0;
	for(std::vector<VModelServer*>::iterator it=servers_.begin(); it!= servers_.end(); ++it)
	{
		if((*it)->server_ == item->serverHandler())
		{
            int nodeNum=(*it)->nodeNum();

#ifdef _UI_VMODELDATA_DEBUG
            UiLog().dbg() << " emit serverRemoveBegin()";
#endif
            //Notifies the model that a change will happen
            Q_EMIT serverRemoveBegin(*it,nodeNum);

			delete *it;
			servers_.erase(it);
            serverNum_=servers_.size();

#ifdef _UI_VMODELDATA_DEBUG
            UiLog().dbg() << " emit serverRemoveEnd()";
#endif
			//Notifies the model that the change has finished
            Q_EMIT serverRemoveEnd(nodeNum);
            return;
		}
		i++;
    }
}

void VModelData::notifyServerFilterChanged(ServerItem* item)
{
	//Q_EMIT dataChanged();
}

void VModelData::notifyServerFilterDelete()
{
#ifdef _UI_VMODELDATA_DEBUG
    UI_FUNCTION_LOG
#endif

#ifdef _UI_VMODELDATA_DEBUG
    UiLog().dbg() << " emits filterDeleteBegin()";
#endif
    Q_EMIT filterDeleteBegin();

	clear();

#ifdef _UI_VMODELDATA_DEBUG
    UiLog().dbg() << " emits filterDeleteEnd()";
#endif

	Q_EMIT filterDeleteEnd();
}

//Should only be called once at the beginning
void VModelData::setActive(bool active)
{
    if(active != active_)
    {
        active_=active;
        if(active_)
            reload();
        else
            clear();
    }
}

void VModelData::reload()
{
#ifdef _UI_VMODELDATA_DEBUG
    UI_FUNCTION_LOG
#endif

    Q_ASSERT(active_);

    for(int i=0; i < serverNum_; i++)
    {
        servers_[i]->reload();
    }
}

void VModelData::slotFilterDefChanged()
{
#ifdef _UI_VMODELDATA_DEBUG
    UI_FUNCTION_LOG
#endif

    if(active_)
        reload();
}

bool VModelData::isFilterComplete() const
{
    for(int i=0; i < serverNum_; i++)
    {      
        return servers_[i]->filter()->isComplete();
    }

    return true;
}

bool VModelData::isFilterNull() const
{
    for(int i=0; i < serverNum_; i++)
    {
        return servers_[i]->filter()->isNull();
    }

    return true;
}

//==============================================================
//
// VTreeModelData
//
//==============================================================

VTreeModelData::VTreeModelData(NodeFilterDef* filterDef,AttributeFilter* attrFilter,AbstractNodeModel* model) :
        VModelData(filterDef,model),
        attrFilter_(attrFilter)
{
    //Attribute filter changes
    connect(attrFilter_,SIGNAL(changed()),
            this,SLOT(slotAttrFilterChanged()));
}

void VTreeModelData::connectToModel(VModelServer* s)
{
    VModelData::connectToModel(s);

    VTreeServer* ts=s->treeServer();
    Q_ASSERT(ts);

    connect(ts,SIGNAL(beginAddRemoveAttributes(VTreeServer*,const VTreeNode*,int,int)),
        model_,SLOT(slotBeginAddRemoveAttributes(VTreeServer*,const VTreeNode*,int,int)));

    connect(ts,SIGNAL(endAddRemoveAttributes(VTreeServer*,const VTreeNode*,int,int)),
        model_,SLOT(slotEndAddRemoveAttributes(VTreeServer*,const VTreeNode*,int,int)));

    connect(ts,SIGNAL(nodeChanged(VTreeServer*,const VTreeNode*)),
        model_,SLOT(slotNodeChanged(VTreeServer*,const VTreeNode*)));

    connect(ts,SIGNAL(attributesChanged(VTreeServer*,const VTreeNode*)),
        model_,SLOT(slotAttributesChanged(VTreeServer*,const VTreeNode*)));

    connect(ts,SIGNAL(beginFilterUpdateRemove(VTreeServer*,const VTreeNode*,int)),
            model_,SLOT(slotBeginFilterUpdateRemove(VTreeServer*,const VTreeNode*,int)));

    connect(ts,SIGNAL(endFilterUpdateRemove(VTreeServer*,const VTreeNode*,int)),
            model_,SLOT(slotEndFilterUpdateRemove(VTreeServer*,const VTreeNode*,int)));

    connect(ts,SIGNAL(beginFilterUpdateAdd(VTreeServer*,const VTreeNode*,int)),
            model_,SLOT(slotBeginFilterUpdateAdd(VTreeServer*,const VTreeNode*,int)));

    connect(ts,SIGNAL(endFilterUpdateAdd(VTreeServer*,const VTreeNode*,int)),
            model_,SLOT(slotEndFilterUpdateAdd(VTreeServer*,const VTreeNode*,int)));

    connect(ts,SIGNAL(beginFilterUpdateRemoveTop(VTreeServer*,int)),
            model_,SLOT(slotBeginFilterUpdateRemoveTop(VTreeServer*,int)));

    connect(ts,SIGNAL(endFilterUpdateRemoveTop(VTreeServer*,int)),
            model_,SLOT(slotEndFilterUpdateRemoveTop(VTreeServer*,int)));

    connect(ts,SIGNAL(beginFilterUpdateInsertTop(VTreeServer*,int)),
            model_,SLOT(slotBeginFilterUpdateInsertTop(VTreeServer*,int)));

    connect(ts,SIGNAL(endFilterUpdateInsertTop(VTreeServer*,int)),
            model_,SLOT(slotEndFilterUpdateInsertTop(VTreeServer*,int)));
}

void VTreeModelData::add(ServerHandler *server)
{
	VModelServer* d=NULL;

    d=new VTreeServer(server,filterDef_,attrFilter_);

    connectToModel(d);

    VModelData::addToServers(d);

    //??????
    if(active_)
        reload();
}

void VTreeModelData::slotAttrFilterChanged()
{
    for(int i=0; i < serverNum_; i++)
    {
        servers_[i]->treeServer()->attrFilterChanged();
    }
}

void VTreeModelData::deleteExpandState()
{
    for(int i=0; i < serverNum_; i++)
    {
        servers_[i]->treeServer()->deleteExpandState();
    }
}

//==============================================================
//
// VTableModelData
//
//==============================================================

VTableModelData::VTableModelData(NodeFilterDef* filterDef,AbstractNodeModel* model) :
		VModelData(filterDef,model)
{
}
void VTableModelData::connectToModel(VModelServer* s)
{
    VModelData::connectToModel(s);

    VTableServer* ts=s->tableServer();
    Q_ASSERT(ts);

    connect(ts,SIGNAL(nodeChanged(VTableServer*,const VNode*)),
        model_,SLOT(slotNodeChanged(VTableServer*,const VNode*)));

}
void VTableModelData::add(ServerHandler *server)
{
	VModelServer* d=NULL;

	d=new VTableServer(server,filterDef_);

    connectToModel(d);

    VModelData::addToServers(d);

    if(active_)
        reload();
}

//Gives the position of this server in the full list of filtered nodes.
int VTableModelData::position(VTableServer* server)
{
	int start=-1;

	if(server)
	{
		start=0;
        for(int i=0; i < serverNum_; i++)
		{
            if(servers_[i] == server)
			{
                return start;
			}          
            start+=servers_[i]->nodeNum();
		}
	}

	return start;
}

//Identifies the range of nodes belonging to this server in the full list of filtered nodes.
bool VTableModelData::position(VTableServer* server,int& start,int& count)
{
	start=-1;
	count=-1;

	if(server)
	{
        if(server->nodeNum() > 0)
		{          
            count=server->nodeNum();
			start=0;
            for(int i=0; i < serverNum_; i++)
			{
                if(servers_[i] == server)
				{
					return true;
				}
                start+=servers_[i]->nodeNum();
			}
		}
	}

	return false;
}

//Gets the position of the given node in the full list of filtered nodes.
//This has to be very fast!!!
int VTableModelData::position(VTableServer* server,const VNode *node) const
{
	if(server)
	{
		int totalRow=0;
        for(int i=0; i < serverNum_; i++)
		{
            if(servers_[i] == server)
			{				
                int pos=server->tableServer()->indexOf(node);
				if(pos != -1)
				{
					totalRow+=pos;
					return totalRow;
				}
				else
					return -1;
			}
			else
			{
                totalRow+=servers_[i]->nodeNum();
			}
		}
	}

	return -1;
}

//This has to be very fast!!!
int VTableModelData::position(const VNode *node) const
{
	int serverIdx=indexOfServer(node->server());
	if(serverIdx != -1)
	{
        return position(servers_[serverIdx]->tableServer(),node);
	}

	return -1;
}

VNode* VTableModelData::nodeAt(int totalRow)
{
	int cnt=0;

	if(totalRow < 0)
		return NULL;

    for(int i=0; i < serverNum_; i++)
	{
		int pos=totalRow-cnt;
        if(pos < servers_[i]->nodeNum())
		{
            return servers_[i]->tableServer()->nodeAt(pos);
		}
        cnt+= servers_[i]->nodeNum();
	}

	return NULL;
}
