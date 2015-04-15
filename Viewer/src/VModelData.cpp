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

//==========================================
//
// NodeModelData
//
//==========================================

//It takes ownership of the filter

VModelServer::VModelServer(ServerHandler *server) :
   server_(server),
   filter_(0)
{
	//We has to observe the nodes of the server.
	server_->addNodeObserver(this);
}

VModelServer::~VModelServer()
{
	server_->removeNodeObserver(this);

	if(filter_)
		delete filter_;
}

VNode* VModelServer::topLevelNode(int row) const
{
  /* server alive but empty 20150410 */
  return server_->vRoot() ? server_->vRoot()->childAt(row) : NULL;
}

int VModelServer::indexOfTopLevelNode(const VNode* node) const
{
  return server_->vRoot() ? server_->vRoot()->indexOfChild(node) : 0;
}

int VModelServer::topLevelNodeNum() const
{
  return server_->vRoot() ? server_->vRoot()->numOfChildren() : 0;
}

int VModelServer::totalNodeNum() const
{
  return server_->vRoot() ? server_->vRoot()->totalNum() : 0;
}

void VModelServer::runFilter()
{
	if(filter_)
		filter_->reset(server_);

}
//==========================================
//
// TreeNodeModelData
//
//==========================================

//It takes ownership of the filter

VTreeServer::VTreeServer(ServerHandler *server,NodeFilterDef* filterDef) :
   VModelServer(server)
{
	filter_=new TableNodeFilter(filterDef);

	//We has to observe the nodes of the server.
	server_->addNodeObserver(this);
}

VTreeServer::~VTreeServer()
{
}

int VTreeServer::checkAttributeUpdateDiff(VNode *node)
{
	int last=node->cachedAttrNum();
	int current=node->attrNum();
	return current-last;
}


void VTreeServer::notifyNodeChanged(const VNode* node, const std::vector<ecf::Aspect::Type>& aspect, const VNodeChange& change)
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
			Q_EMIT addRemoveAttributes(this,node,
					change.attrNum_,change.cachedAttrNum_);
		}
	}

	//----------------------------------------------------------------------
	// The number of nodes changed but number of attributes is the same!
	//----------------------------------------------------------------------
	else if(!attrNumCh && nodeNumCh)
	{
		//Only one node was added and the order of the nodes is the same
		if(change.nodeAddedAt_ != -1)
		{
			Q_EMIT addNode(this,node,change.nodeAddedAt_);
		}
		else
		{
			Q_EMIT resetBranch(this,node);
		}

		runFilter=true;
	}

	//---------------------------------------------------------------------
	// Both the number of nodes and the number of attributes changed!
	//---------------------------------------------------------------------
	else if(!attrNumCh && nodeNumCh)
	{
		Q_EMIT resetBranch(this,node);

		runFilter=true;
	}

	//---------------------------------------------------------------------
	// The number of attributes and nodes did not change
	//---------------------------------------------------------------------
	else
	{
		//Check the aspects
		for(std::vector<ecf::Aspect::Type>::const_iterator it=aspect.begin(); it != aspect.end(); it++)
		{
			//Changes in the nodes
			if(*it == ecf::Aspect::STATE || *it == ecf::Aspect::DEFSTATUS ||
			   *it == ecf::Aspect::SUSPENDED)
			{
				Q_EMIT nodeChanged(this,node);
				runFilter=true;
			}

			//Changes in the attributes
			else if(*it == ecf::Aspect::METER ||  *it == ecf::Aspect::EVENT ||
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

	//We need to check if it changes the filter state
	if(runFilter && filter_)
	{
		/*if(filter_->update(node->node()))
		{
			Q_EMIT dataChanged(this);
		}*/
	}
}

//==========================================
//
// TableNodeModelData
//
//==========================================

//It takes ownership of the filter

VTableServer::VTableServer(ServerHandler *server,NodeFilterDef* filterDef) :
	VModelServer(server)

{
	filter_=new TableNodeFilter(filterDef);

	//We has to observe the nodes of the server.
	server_->addNodeObserver(this);
}

VTableServer::~VTableServer()
{
}

void VTableServer::notifyNodeChanged(const VNode* node, const std::vector<ecf::Aspect::Type>& types,const VNodeChange&)
{
}


//==========================================
//
// NodeModelDataHandler
//
//==========================================

VModelData::VModelData(ServerFilter* serverFilter,NodeFilterDef *filterDef,ModelType modelType) :
		modelType_(modelType),
		serverFilter_(serverFilter),
		filterDef_(filterDef)
{
	connect(filterDef_,SIGNAL(changed()),
			this,SLOT(slotFilterDefChanged()));

	init();
}


VModelData::~VModelData()
{
	clear();
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

void VModelData::add(ServerHandler *server)
{
	VModelServer* d=NULL;

	switch(modelType_)
	{
	case TreeModel:
		d=new VTreeServer(server,filterDef_);
		break;
	case TableModel:
		d=new VTableServer(server,filterDef_);
		break;
	default:
		return; //TODO: give an error message!!
	}

	connect(d,SIGNAL(dataChanged(VModelServer*)),
			this,SIGNAL(dataChanged(VModelServer*)));

	connect(d,SIGNAL(nodeChanged(VModelServer*,const VNode*)),
			  this,SIGNAL(nodeChanged(VModelServer*,const VNode*)));

	connect(d,SIGNAL(attributesChanged(VModelServer*,const VNode*)),
				  this,SIGNAL(attributesChanged(VModelServer*,const VNode*)));

	connect(d,SIGNAL(addRemoveAttributes(VModelServer*,const VNode*,int,int)),
			this,SIGNAL(addRemoveAttributes(VModelServer*,const VNode*,int,int)));

	connect(d,SIGNAL(addRemoveNodes(VModelServer*,const VNode*,int,int)),
				this,SIGNAL(addRemoveNodes(VModelServer*,const VNode*,int,int)));

	connect(d,SIGNAL(addNode(VModelServer*,const VNode*,int)),
					this,SIGNAL(addNode(VModelServer*,const VNode*,int)));

	connect(d,SIGNAL(resetBranch(VModelServer*,const VNode*)),
						this,SIGNAL(resetBranch(VModelServer*,const VNode*)));

	servers_.push_back(d);
}

VModelServer* VModelData::server(int n) const
{
	return (n >=0 && n < servers_.size())?servers_.at(n):0;
}

ServerHandler* VModelData::realServer(int n) const
{
	return (n >=0 && n < servers_.size())?servers_.at(n)->server_:0;
}

/*ServerHandler* VModelData::server(void* idPointer) const
{
	for(unsigned int i=0; i < servers_.size(); i++)
		if(servers_.at(i)->server_ == idPointer)
			return servers_.at(i)->server_;

	return NULL;
}*/

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

//This has to be very fast!!!
bool VModelData::isFiltered(VNode *node) const
{
	/*ServerHandler* server=ServerHandler::find(node);
	int id=indexOf(server);
	if(id != -1 && servers_.at(id)->filter_)
	{
		return servers_.at(id)->filter_->isFiltered(node);
	}*/

	return true;
}

int VModelData::numOfNodes(int index) const
{
	if(VModelServer *d=server(index))
	{
		return d->totalNodeNum();
	}
	return 0;
}

VNode* VModelData::getNodeFromFilter(int totalRow)
{
	int cnt=0;

	for(unsigned int i=0; i < servers_.size(); i++)
	{
		NodeFilter *filter=servers_.at(i)->filter_;
		if(totalRow-cnt < filter->matchCount())
		{
			return NULL;//filter->match(totalRow-cnt);
		}
		cnt+=filter->matchCount();
	}

	return NULL;
}

int VModelData::numOfFiltered(int index) const
{
	if(VModelServer *d=server(index))
	{
		if(d->filter_)
			return d->filter_->matchCount();
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

VNode* VModelData::topLevelNode(void* ptrToServer,int row)
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

bool VModelData::identifyTopLevelNode(const VNode* node,VModelServer** server,int& row)
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
	for(std::vector<VModelServer*>::iterator it=servers_.begin(); it!= servers_.end(); it++)
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


//Node observer
/*
void NodeModelDataHandler::notifyNodeChanged(const Node* node, const std::vector<ecf::Aspect::Type>& types)
{
	if(node==NULL)
		return;

	qDebug() << "observer is called" << QString::fromStdString(node->name());
	//for(unsigned int i=0; i < types.size(); i++)
	//	qDebug() << "  type:" << types.at(i);

	Node* nc=const_cast<Node*>(node);

	QModelIndex index1=nodeToIndex(nc,0);
	QModelIndex index2=nodeToIndex(nc,2);

	if(!index1.isValid() || !index2.isValid())
		return;

	Node *nd1=indexToNode(index1);
	Node *nd2=indexToNode(index2);

	if(!nd1 || !nd2)
		return;

	//qDebug() << "indexes" << index1 << index2;
	//qDebug() << "index pointers " << index1.internalPointer() << index2.internalPointer();
	qDebug() << "    --->" << QString::fromStdString(nd1->name()) << QString::fromStdString(nd2->name());

	Q_EMIT dataChanged(index1,index2);
}
*/




/*
VModelServer* TreeNodeModelDataHandler::makeData(ServerHandler* s)
{
	return new VTreeServer(s,filterDef_);
}

VModelServer* TableNodeModelDataHandler::makeData(ServerHandler* s)
{
	return new VTableServer(s,filterDef_);
}
*/


