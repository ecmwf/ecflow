//============================================================================
// Copyright 2015 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "VModelData.hpp"

#include "VFilter.hpp"
#include "ServerHandler.hpp"
#include "VAttribute.hpp"
#include "VNode.hpp"

#include  "AbstractNodeModel.hpp"

#include <QDebug>
#include <QMetaMethod>

//==========================================
//
// VModelServer
//
//==========================================

//It takes ownership of the filter

VModelServer::VModelServer(ServerHandler *server) :
   server_(server),
   filter_(0)
{
	//We has to observe the nodes of the server.
	server_->addNodeObserver(this);

	//We has to observe the nodes of the server.
	server_->addServerObserver(this);

}

VModelServer::~VModelServer()
{
	server_->removeNodeObserver(this);
	server_->removeServerObserver(this);

	if(filter_)
		delete filter_;
}

VNode* VModelServer::topLevelNode(int row) const
{
  return server_->vRoot()->childAt(row);
}

int VModelServer::indexOfTopLevelNode(const VNode* node) const
{
  return server_->vRoot()->indexOfChild(node);
}

int VModelServer::topLevelNodeNum() const
{
  return server_->vRoot()->numOfChildren();
}

int VModelServer::totalNodeNum() const
{
  return server_->vRoot()->totalNum();
}

void VModelServer::runFilter()
{
	if(filter_)
	{
		filter_->beginReset(server_);
		filter_->endReset();
	}
}

//==========================================
//
// VTreeServer
//
//==========================================

//It takes ownership of the filter

VTreeServer::VTreeServer(ServerHandler *server,NodeFilterDef* filterDef) :
   VModelServer(server),
   nodeStateChangeCnt_(0)
{
	filter_=new TreeNodeFilter(filterDef);

	//We has to observe the nodes of the server.
	//server_->addNodeObserver(this);
}

VTreeServer::~VTreeServer()
{
}

int VTreeServer::checkAttributeUpdateDiff(VNode *node)
{
	//int last=node->cachedAttrNum();
	//int current=node->attrNum();
	//return current-last;
	return 0;
}

//--------------------------------------------------
// ServerObserver methods
//--------------------------------------------------

void VTreeServer::notifyDefsChanged(ServerHandler* server, const std::vector<ecf::Aspect::Type>& a)
{
	Q_EMIT dataChanged(this);
}

void VTreeServer::notifyBeginServerScan(ServerHandler* server,const VServerChange& change)
{
	Q_EMIT beginServerScan(this,change.suiteNum_);
}

void VTreeServer::notifyEndServerScan(ServerHandler* server)
{
	runFilter();
	Q_EMIT endServerScan(this,server->vRoot()->numOfChildren());
}

void VTreeServer::notifyBeginServerClear(ServerHandler* server)
{
	Q_EMIT beginServerClear(this,-1);
}

void VTreeServer::notifyEndServerClear(ServerHandler* server)
{
	runFilter();
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

//This is called when a normal sync (no reset or rescan) is finished. We have delayed the update of the
//filter to this point but now we need to do it.
void VTreeServer::notifyEndServerSync(ServerHandler* server)
{
	if(nodeStateChangeCnt_ >0)
	{
		nodeStateChangeCnt_=0;

		if(!filter_->isNull())
		{
			runFilter();
			Q_EMIT filterChanged();
		}
	}
}

//--------------------------------------------------
// NodeObserver methods
//--------------------------------------------------

void VTreeServer::notifyBeginNodeChange(const VNode* node, const std::vector<ecf::Aspect::Type>& aspect, const VNodeChange& change)
{
	if(node==NULL)
		return;

	bool runFilter=false;
	bool attrNumCh=(std::find(aspect.begin(),aspect.end(),ecf::Aspect::ADD_REMOVE_ATTR) != aspect.end());
	bool nodeNumCh=(std::find(aspect.begin(),aspect.end(),ecf::Aspect::ADD_REMOVE_NODE) != aspect.end());

	//-----------------------------------------------------------------------
	// The number of attributes changed but the number of nodes is the same!
	//-----------------------------------------------------------------------
	if(attrNumCh && !nodeNumCh)
	{
		if(change.cachedAttrNum_ != change.attrNum_)
		{
			Q_EMIT beginAddRemoveAttributes(this,node,
					change.attrNum_,change.cachedAttrNum_);
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
				Q_EMIT nodeChanged(this,node);
				runFilter=true;
			}

			//Changes might affect the icons
			else if (*it == ecf::Aspect::FLAG || *it == ecf::Aspect::SUBMITTABLE ||
					*it == ecf::Aspect::TODAY || *it == ecf::Aspect::TIME ||
					*it == ecf::Aspect::DAY || *it == ecf::Aspect::CRON || *it == ecf::Aspect::DATE)
			{
				Q_EMIT nodeChanged(this,node);
			}

			//Changes in the attributes
			if(*it == ecf::Aspect::METER ||  *it == ecf::Aspect::EVENT ||
			   *it == ecf::Aspect::LABEL || *it == ecf::Aspect::LIMIT ||
			   *it == ecf::Aspect::EXPR_TRIGGER || *it == ecf::Aspect::EXPR_COMPLETE ||
			   *it == ecf::Aspect::REPEAT || *it == ecf::Aspect::NODE_VARIABLE ||
			   *it == ecf::Aspect::LATE || *it == ecf::Aspect::TODAY || *it == ecf::Aspect::TIME ||
			   *it == ecf::Aspect::DAY || *it == ecf::Aspect::CRON || *it == ecf::Aspect::DATE )
			{
				Q_EMIT attributesChanged(this,node);
			}
		}
	}

	//We do not run the filter now but wait until the sync is finished!
	if(runFilter)
	{
		nodeStateChangeCnt_++;
	}
}

void VTreeServer::notifyEndNodeChange(const VNode* node, const std::vector<ecf::Aspect::Type>& aspect, const VNodeChange& change)
{
	if(node==NULL)
		return;

	bool attrNumCh=(std::find(aspect.begin(),aspect.end(),ecf::Aspect::ADD_REMOVE_ATTR) != aspect.end());
	bool nodeNumCh=(std::find(aspect.begin(),aspect.end(),ecf::Aspect::ADD_REMOVE_NODE) != aspect.end());

	//-----------------------------------------------------------------------
	// The number of attributes changed but the number of nodes is the same!
	//-----------------------------------------------------------------------
	if(attrNumCh && !nodeNumCh)
	{
		if(change.cachedAttrNum_ != change.attrNum_)
		{
			Q_EMIT endAddRemoveAttributes(this,node,
					change.attrNum_,change.cachedAttrNum_);
		}
	}

	//----------------------------------------------------------------------
	// The number of nodes changed but number of attributes is the same!
	//----------------------------------------------------------------------
	else if(!attrNumCh && nodeNumCh)
	{
		assert(0);
	}

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
	filter_=new TableNodeFilter(filterDef);

	//We has to observe the nodes of the server.
	//server_->addNodeObserver(this);
}

VTableServer::~VTableServer()
{
}

//--------------------------------------------------
// ServerObserver methods
//--------------------------------------------------


void VTableServer::notifyBeginServerScan(ServerHandler* server,const VServerChange& change)
{
	//At this point we do not know how many nodes we will have in the filter!
}

void VTableServer::notifyEndServerScan(ServerHandler* server)
{
	//Update the filter using the current server state
	filter_->beginReset(server);

	int realCount=filter_->realMatchCount();

	//The filter pretends it is empty
	//Tell the model to add realCount number of rows!!
	Q_EMIT beginServerScan(this,realCount);

	filter_->endReset();

	Q_EMIT endServerScan(this,realCount);
}

void VTableServer::notifyBeginServerClear(ServerHandler* server)
{
	int n=filter_->matchCount();
	Q_EMIT beginServerClear(this,n);
}

void VTableServer::notifyEndServerClear(ServerHandler* server)
{
	int n=filter_->matchCount();
	filter_->clear();
	Q_EMIT endServerClear(this,n);
}

void VTableServer::notifyServerConnectState(ServerHandler* server)
{
	Q_EMIT rerender();
}

void VTableServer::notifyServerActivityChanged(ServerHandler* server)
{
	Q_EMIT dataChanged(this);
}

void VTableServer::notifyBeginNodeChange(const VNode* node, const std::vector<ecf::Aspect::Type>& types,const VNodeChange&)
{
	int n=filter_->matchCount();
	Q_EMIT beginServerClear(this,n);
	filter_->clear();
	Q_EMIT endServerClear(this,n);
}

void VTableServer::notifyEndNodeChange(const VNode* node, const std::vector<ecf::Aspect::Type>& types,const VNodeChange&)
{
	//Update the filter using the current server state
	filter_->beginReset(server_);
	int realCount=filter_->matchCount();

	//Tell the model to add realCount number of rows!!
	Q_EMIT beginServerScan(this,realCount);

	filter_->endReset();

	Q_EMIT endServerScan(this,realCount);

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
		model_(model)
{
	connect(filterDef_,SIGNAL(changed()),
			this,SLOT(slotFilterDefChanged()));

	//The model relays this signal
	connect(this,SIGNAL(filterChanged()),
			model_,SIGNAL(filterChanged()));

	connect(this,SIGNAL(serverAddBegin(int)),
			model_,SLOT(slotServerAddBegin(int)));

	connect(this,SIGNAL(serverAddEnd()),
			model_,SLOT(slotServerAddEnd()));

	connect(this,SIGNAL(serverRemoveBegin(int)),
			model_,SLOT(slotServerRemoveBegin(int)));

	connect(this,SIGNAL(serverRemoveEnd()),
			model_,SLOT(slotServerRemoveEnd()));
}

VModelData::~VModelData()
{
	clear();
}

void VModelData::connectToModel(VModelServer* d)
{
	connect(d,SIGNAL(beginAddRemoveAttributes(VModelServer*,const VNode*,int,int)),
		model_,SLOT(slotBeginAddRemoveAttributes(VModelServer*,const VNode*,int,int)));

	connect(d,SIGNAL(endAddRemoveAttributes(VModelServer*,const VNode*,int,int)),
		model_,SLOT(slotEndAddRemoveAttributes(VModelServer*,const VNode*,int,int)));

	connect(d,SIGNAL(dataChanged(VModelServer*)),
		model_,SLOT(slotDataChanged(VModelServer*)));

	connect(d,SIGNAL(nodeChanged(VModelServer*,const VNode*)),
		model_,SLOT(slotNodeChanged(VModelServer*,const VNode*)));

	connect(d,SIGNAL(attributesChanged(VModelServer*,const VNode*)),
		model_,SLOT(slotAttributesChanged(VModelServer*,const VNode*)));

	connect(d,SIGNAL(beginServerScan(VModelServer*,int)),
		model_,SLOT(slotBeginServerScan(VModelServer*,int)));

	connect(d,SIGNAL(endServerScan(VModelServer*,int)),
		model_,SLOT(slotEndServerScan(VModelServer*,int)));

	connect(d,SIGNAL(beginServerClear(VModelServer*,int)),
		model_,SLOT(slotBeginServerClear(VModelServer*,int)));

	connect(d,SIGNAL(endServerClear(VModelServer*,int)),
		model_,SLOT(slotEndServerClear(VModelServer*,int)));

	//The model relays this signal
	connect(d,SIGNAL(filterChanged()),
				model_,SIGNAL(filterChanged()));

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
	if(serverFilter_)
		serverFilter_->removeObserver(this);

	for(int i=0; i < servers_.size(); i++)
	{
		delete servers_.at(i);
	}

	servers_.clear();
}

VModelServer* VModelData::server(int n) const
{
	return (n >=0 && n < servers_.size())?servers_.at(n):0;
}

ServerHandler* VModelData::realServer(int n) const
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
		return d->totalNodeNum();
	}
	return 0;
}

void VModelData::runFilter(bool broadcast)
{
	for(unsigned int i=0; i < servers_.size(); i++)
	{
		servers_.at(i)->runFilter();
	}

	if(broadcast)
	{
		Q_EMIT filterChanged();
	}
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
			//Notifies the model that a change will happen
			Q_EMIT serverRemoveBegin(i);

			delete *it;
			servers_.erase(it);

			//Notifies the model that the change has finished
			Q_EMIT serverRemoveEnd();
			return;
		}
		i++;
	}
}

void VModelData::notifyServerFilterChanged(ServerItem* item)
{
	//Q_EMIT dataChanged();
}

void VModelData::slotFilterDefChanged()
{
	runFilter(true);
}

//==============================================================
//
// VTreeModelData
//
//==============================================================

VTreeModelData::VTreeModelData(NodeFilterDef* filterDef,AbstractNodeModel* model) :
		VModelData(filterDef,model)
{

}

void VTreeModelData::add(ServerHandler *server)
{
	VModelServer* d=NULL;

	d=new VTreeServer(server,filterDef_);

	VModelData::connectToModel(d);

	servers_.push_back(d);
}

VNode* VTreeModelData::topLevelNode(void* ptrToServer,int row)
{
	for(unsigned int i=0; i < servers_.size(); i++)
	{
		if(servers_.at(i) == ptrToServer)
		{
			return servers_.at(i)->topLevelNode(row);
		}
	}
	return NULL;
}

bool VTreeModelData::identifyTopLevelNode(const VNode* node,VModelServer** server,int& row)
{
	for(unsigned int i=0; i < servers_.size(); i++)
	{
		row=servers_.at(i)->indexOfTopLevelNode(node);
		if(row != -1)
		{
			*server=servers_.at(i);
			return true;
		}
	}

	return false;
}

//This has to be very fast!!!
bool VTreeModelData::isFiltered(VNode *node) const
{
	int id=indexOfServer(node->server());
	if(id != -1)
	{
		return servers_.at(id)->filter_->isFiltered(node);
	}

	return true;
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

void VTableModelData::add(ServerHandler *server)
{
	VModelServer* d=NULL;

	d=new VTableServer(server,filterDef_);

	VModelData::connectToModel(d);

	servers_.push_back(d);
}


bool VTableModelData::identifyInFilter(VModelServer* server,int& start,int& count,VNode** node)
{
	start=-1;
	count=-1;
	*node=NULL;

	if(server)
	{
		*node=server->filter_->realMatchAt(0);
		if(*node)
		{
			count=server->filter_->realMatchCount();
			start=0;
			for(unsigned int i=0; i < servers_.size(); i++)
			{
				if(servers_.at(i) == server)
				{
					return true;
				}
				start+=servers_.at(i)->filter_->matchCount();
			}
		}
	}

	return false;
}

//This has to be very fast!!!
int VTableModelData::posInFilter(VModelServer* server,const VNode *node) const
{
	if(server)
	{
		int totalRow=0;
		for(unsigned int i=0; i < servers_.size(); i++)
		{
			NodeFilter *filter=servers_.at(i)->filter_;
			if(servers_.at(i) == server)
			{
				int pos=filter->matchPos(node);
				if(pos != -1)
				{
					totalRow+=filter->matchPos(node);
					return totalRow;
				}
				else
					return -1;
			}
			else
			{
				totalRow+=filter->matchCount();
			}
		}
	}

	return -1;
}

//This has to be very fast!!!
int VTableModelData::posInFilter(const VNode *node) const
{
	int serverIdx=indexOfServer(node->server());
	if(serverIdx != -1)
	{
		return posInFilter(servers_.at(serverIdx),node);
	}

	return -1;
}

VNode* VTableModelData::getNodeFromFilter(int totalRow)
{
	int cnt=0;

	if(totalRow < 0)
		return NULL;

	for(unsigned int i=0; i < servers_.size(); i++)
	{
		NodeFilter *filter=servers_.at(i)->filter_;
		int pos=totalRow-cnt;
		if(pos < filter->matchCount())
		{
			return filter->matchAt(pos);
		}
		cnt+=filter->matchCount();
	}

	return NULL;
}

//Return the number of filtered nodes for the given server
int VTableModelData::numOfFiltered(int index) const
{
	if(VModelServer *d=server(index))
	{
		return d->filter_->matchCount();

	}
	return 0;
}

//This has to be very fast!!!
bool VTableModelData::isFiltered(VNode *node) const
{
	return true;
}
