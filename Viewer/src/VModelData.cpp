//============================================================================
// Copyright 2015 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "VModelData.hpp"

#include "AbstractNodeModel.hpp"
#include "NodeQuery.hpp"
#include "VFilter.hpp"
#include "ServerHandler.hpp"
#include "UserMessage.hpp"
#include "VAttribute.hpp"
#include "VNode.hpp"
#include "VTree.hpp"

#include <QDebug>
#include <QMetaMethod>

//#define _UI_VMODELDATA_DEBUG

void VTreeChangeInfo::addStateChange(const VNode* n)
{
    VNode* s=n->suite();
    assert(s);
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
   attrFilter_(attrFilter)
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
}

NodeFilter* VTreeServer::filter() const
{
    return const_cast<TreeNodeFilter*>(filter_);
}

int VTreeServer::nodeNum() const
{
    return tree_->totalNum();
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

void VTreeServer::notifyBeginServerScan(ServerHandler* server,const VServerChange& change)
{
    //When the server scan begins we must be in inScan mode so that the model should think that
    //this server tree is empty.
    inScan_=true;
    Q_ASSERT(tree_->numOfChildren() == 0);
}

void VTreeServer::notifyEndServerScan(ServerHandler* /*server*/)
{
    //We still must be in inScan mode so that the model should think
    //that this server tree is empty.
    inScan_=true;

    //When the server scan ends we need to rebuild the tree.
    if(filter_->isNull())
    {
        tree_->build();
    }
    else
    {
        filter_->update();
        tree_->build(filter_->match_);
    }

    //Notifies the model of the number of children nodes to be added to the server node.
    Q_EMIT beginServerScan(this, tree_->attrNum(attrFilter_)+tree_->numOfChildren());
    //We leave the inScan mode. From this moment on the model can see the whole tree in the server.
    inScan_=false;
    //Notifies the model that the scan finished. The model can now relayout its new contents.
    Q_EMIT endServerScan(this, tree_->attrNum(attrFilter_)+tree_->numOfChildren());
}

void VTreeServer::notifyBeginServerClear(ServerHandler* server)
{
    Q_EMIT beginServerClear(this,-1);
    tree_->clear();
    filter_->clear();
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
        UserMessage::debug("VTreeServer::notifyEndServerSync --> number of state changes: " + QString::number(changeInfo_->stateChangeSuites().size()).toStdString());
#endif

    //if there was a state change during the sync
    if(changeInfo_->stateChangeSuites().size() > 0 && !filter_->isNull())
	{
#ifdef _UI_VMODELDATA_DEBUG
        UserMessage::debug("  Suites changed:");
        for(size_t i= 0; i < changeInfo_->stateChangeSuites().size(); i++)
            UserMessage::debug("     " +  changeInfo_->stateChangeSuites().at(i)->strName());
#endif
        //Update the filter for the suites with a status change. topFilterChange
        //will contain the branches where the filter changed. A branch is
        //defined by the parent of the top level nodes with a filter status change in a given
        //suite. Suites are always visible (part of the tree) so a branch cannot be a server (root)
        //but at most a suite.
        std::vector<VNode*> topFilterChange;
        filter_->update(changeInfo_->stateChangeSuites(),topFilterChange);

#ifdef _UI_VMODELDATA_DEBUG
        UserMessage::debug("  Top level nodes that changed in filter:");
        for(size_t i= 0; i < topFilterChange.size(); i++)
            UserMessage::debug("     " +  topFilterChange.at(i)->strName());
#endif

        //A topFilterChange branch cannot be the root (server)
        for(size_t i=0; i < topFilterChange.size(); i++)
        {
            Q_ASSERT(!topFilterChange[i]->isServer());
        }

        //If something changed in the list of filtered nodes
        for(size_t i=0; i < topFilterChange.size(); i++)
        {
#ifdef _UI_VMODELDATA_DEBUG
            UserMessage::debug("  Branch: " + topFilterChange[i]->absNodePath());
#endif
            //This is the branch where there is a change in the filter
            VTreeNode* tn=tree_->find(topFilterChange[i]);

            //If the branch is not in tree (not yet filtered) we need to
            //find it nearest ancestor up in the tree. This must exist because
            //the suites are always part of the tree.
            if(!tn)
            {
                tn=tree_->findAncestor(topFilterChange[i]);
                Q_ASSERT(tn);
                Q_ASSERT(!tn->isRoot());
            }

#ifdef _UI_VMODELDATA_DEBUG
            UserMessage::debug("  Branch treeNode: " + tn->vnode()->absNodePath());
#endif
            //First, we remove the branch contents
            if(tn->numOfChildren())
            {
                int totalRows=tn->attrNum(attrFilter_) + tn->numOfChildren();
                Q_EMIT beginFilterUpdateRemove(this,tn,totalRows);
                tree_->removeChildren(tn);
                Q_EMIT endFilterUpdateRemove(this,tn,totalRows);
            }

            //Second, we add the new contents
            VTreeNode *branch=new VTreeNode(tn->vnode(),0);
            tree_->buildBranch(filter_->match_,tn,branch);
            int chNum=branch->numOfChildren();

            Q_EMIT beginFilterUpdateAdd(this,tn,chNum);
            if(chNum)
            {
               tree_->addBranch(tn,branch);
            }
            Q_EMIT endFilterUpdateAdd(this,tn,chNum);

            //branch must be empty now
            Q_ASSERT(branch->numOfChildren() == 0);

            delete branch;
        }
    }

    changeInfo_->clear();

#ifdef _UI_VMODELDATA_DEBUG
    UserMessage::debug("<-- VTreeServer::notifyEndServerSync");
#endif
}

//--------------------------------------------------
// NodeObserver methods
//--------------------------------------------------

void VTreeServer::notifyBeginNodeChange(const VNode* vnode, const std::vector<ecf::Aspect::Type>& aspect, const VNodeChange& change)
{
    if(vnode==NULL)
		return;

    VTreeNode* node=tree_->find(vnode);

	bool attrNumCh=(std::find(aspect.begin(),aspect.end(),ecf::Aspect::ADD_REMOVE_ATTR) != aspect.end());
	bool nodeNumCh=(std::find(aspect.begin(),aspect.end(),ecf::Aspect::ADD_REMOVE_NODE) != aspect.end());

	//-----------------------------------------------------------------------
	// The number of attributes changed but the number of nodes is the same!
	//-----------------------------------------------------------------------
    if(node && attrNumCh && !nodeNumCh)
	{
        //We do not deal with the attributes if they were never used for the given node.
        //The first access to the attributes makes them initialised in the tree node.
        if(node->isAttrInitialised())
        {
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

                changeInfo_->addStateChange(vnode);

#ifdef _UI_VMODELDATA_DEBUG
                UserMessage::debug("   node status changed: " + vnode->strName());
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
    Q_EMIT beginServerClear(this,-1);
    tree_->clear();
    inScan_=true;
    Q_EMIT endServerClear(this,-1);

    Q_ASSERT(filter_);

    if(filter_->isNull())
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
    UserMessage::debug("VTableServer::reload --> " + server_->name());
#endif

    int oriNodeNum=nodeNum();
#ifdef _UI_VMODELDATA_DEBUG
    UserMessage::debug("    oriNodeNum: " + QString::number(oriNodeNum).toStdString());
#endif

    Q_EMIT beginServerClear(this,oriNodeNum);
    filter_->clear();
    inScan_=true;
    Q_EMIT endServerClear(this,oriNodeNum);

    filter_->update();

    Q_EMIT beginServerScan(this, nodeNum());
    inScan_=false;
    Q_EMIT endServerScan(this, nodeNum());

#ifdef _UI_VMODELDATA_DEBUG
    UserMessage::debug("    nodeNum: " + QString::number(oriNodeNum).toStdString());
    UserMessage::debug("<-- VTableServer::reload");
#endif


}

//==========================================
//
// VModelData
//
//==========================================

VModelData::VModelData(NodeFilterDef *filterDef,AbstractNodeModel* model) :
		QObject(model),
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

    connect(this,SIGNAL(filterChangeBegun()),
            model_,SIGNAL(filterChangeBegun()));

    connect(this,SIGNAL(filterChangeEnded()),
           model_,SIGNAL(filterChangeEnded()));

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

void VModelData::clear()
{
#ifdef _UI_VMODELDATA_DEBUG
    UserMessage::debug("VModelData::clear --> " +  QString::number((qint64)this,16).toStdString());
    qDebug() << "   serverFilter_" << serverFilter_;
#endif

    if(serverFilter_)
		serverFilter_->removeObserver(this);

    serverFilter_=NULL;

	for(int i=0; i < servers_.size(); i++)
	{
		delete servers_.at(i);
	}

	servers_.clear();

#ifdef _UI_VMODELDATA_DEBUG
    UserMessage::debug("<-- VModelData::clear");
#endif
}

VModelServer* VModelData::server(int n) const
{
	return (n >=0 && n < servers_.size())?servers_.at(n):0;
}

ServerHandler* VModelData::serverHandler(int n) const
{
	return (n >=0 && n < servers_.size())?servers_.at(n)->server_:0;
}

int VModelData::indexOfServer(void* idPointer) const
{
	for(unsigned int i=0; i < servers_.size(); i++)
		if(servers_.at(i) == idPointer)
			return i;

	return -1;
}

ServerHandler* VModelData::serverHandler(void* idPointer) const
{
	for(unsigned int i=0; i < servers_.size(); i++)
		if(servers_.at(i) == idPointer)
			return servers_.at(i)->server_;

	return NULL;
}

VModelServer* VModelData::server(const void* idPointer) const
{
	for(unsigned int i=0; i < servers_.size(); i++)
		if(servers_.at(i) == idPointer)
			return servers_.at(i);

	return NULL;
}

VModelServer* VModelData::server(const std::string& name) const
{
    for(unsigned int i=0; i < servers_.size(); i++)
        if(servers_.at(i)->server_->name()  == name)
            return servers_.at(i);

    return NULL;
}

VModelServer* VModelData::server(ServerHandler* s) const
{
    for(unsigned int i=0; i < servers_.size(); i++)
        if(servers_.at(i)->server_ == s)
            return servers_.at(i);

    return NULL;
}


int VModelData::indexOfServer(ServerHandler* s) const
{
	for(unsigned int i=0; i < servers_.size(); i++)
		if(servers_.at(i)->server_ == s)
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
	if(!item)
		return;

	int i=0;
	for(std::vector<VModelServer*>::iterator it=servers_.begin(); it!= servers_.end(); ++it)
	{
		if((*it)->server_ == item->serverHandler())
		{
            int nodeNum=(*it)->nodeNum();

            //Notifies the model that a change will happen
            Q_EMIT serverRemoveBegin(*it,nodeNum);

			delete *it;
			servers_.erase(it);

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
    UserMessage::debug("VModelData::notifyServerFilterDelete --> " + QString::number((qint64)this,16).toStdString());
#endif

    Q_EMIT filterDeleteBegin();

#ifdef _UI_VMODELDATA_DEBUG
    UserMessage::debug("  filterDeleteBegin emitted");
#endif

	clear();

#ifdef _UI_VMODELDATA_DEBUG
    UserMessage::debug("  filterDeleteEnd emitted");
#endif

	Q_EMIT filterDeleteEnd();

#ifdef _UI_VMODELDATA_DEBUG
    UserMessage::debug("<-- VModelData::notifyServerFilterDelete");
#endif

}

//Should only be called once at the beginning
void VModelData::setActive(bool active)
{
    if(active != active_)
    {
        active_=active;
        if(active_)
            reload(false);
        else
            clear();
    }
}

void VModelData::reload(bool broadcast)
{
    Q_ASSERT(active_);

    if(broadcast)
        Q_EMIT filterChangeBegun();

    for(unsigned int i=0; i < servers_.size(); i++)
    {
        servers_.at(i)->reload();
    }

    if(broadcast)
         Q_EMIT filterChangeEnded();
}

void VModelData::slotFilterDefChanged()
{
    if(active_)
        reload(true);
}

bool VModelData::isFilterNull() const
{
    for(unsigned int i=0; i < servers_.size(); i++)
    {      
        return servers_.at(i)->filter()->isNull();
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
}

void VTreeModelData::add(ServerHandler *server)
{
	VModelServer* d=NULL;

    d=new VTreeServer(server,filterDef_,attrFilter_);

    connectToModel(d);

	servers_.push_back(d);

    if(active_)
        reload(true);
}

void VTreeModelData::slotAttrFilterChanged()
{
    Q_EMIT filterChangeBegun();

    for(unsigned int i=0; i < servers_.size(); i++)
    {
        servers_.at(i)->treeServer()->attrFilterChanged();
    }

    Q_EMIT filterChangeEnded();

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

	servers_.push_back(d);

    if(active_)
        reload(false);
}

//Gives the position of this server in the full list of filtered nodes.
int VTableModelData::position(VTableServer* server)
{
	int start=-1;

	if(server)
	{
		start=0;
		for(unsigned int i=0; i < servers_.size(); i++)
		{
			if(servers_.at(i) == server)
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
			for(unsigned int i=0; i < servers_.size(); i++)
			{
				if(servers_.at(i) == server)
				{
					return true;
				}
                start+=servers_.at(i)->nodeNum();
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
		for(unsigned int i=0; i < servers_.size(); i++)
		{
			if(servers_.at(i) == server)
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
                totalRow+=server->nodeNum();
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

	for(unsigned int i=0; i < servers_.size(); i++)
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
