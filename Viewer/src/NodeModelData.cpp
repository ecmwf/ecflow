//============================================================================
// Copyright 2015 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "NodeModelData.hpp"

#include "VFilter.hpp"
#include "ServerHandler.hpp"

//==========================================
//
// NodeModelData
//
//==========================================

//It takes ownership of the filter

NodeModelData::NodeModelData(ServerHandler *server,NodeFilter* filter) :
   server_(server),
   filter_(filter),
   nodeNum_(-1)
{
	//We has to observe the nodes of the server.
	server_->addNodeObserver(this);
}

NodeModelData::~NodeModelData()
{
	server_->addNodeObserver(this);

	if(filter_)
		delete filter_;
}

int NodeModelData::nodeNum() const
{
	if(nodeNum_==-1)
		nodeNum_=server_->numberOfNodes();

	return nodeNum_;
}

void NodeModelData::runFilter()
{
	if(filter_)
		filter_->reset(server_);

}

void NodeModelData::notifyNodeChanged(const Node* node, const std::vector<ecf::Aspect::Type>& types)
{
	if(node==NULL)
		return;

	//qDebug() << "observer is called" << QString::fromStdString(node->name());
	//for(unsigned int i=0; i < types.size(); i++)
	//	qDebug() << "  type:" << types.at(i);

	Node* nc=const_cast<Node*>(node);

	//We need to check if it changes the filter state
	if(filter_)
	{
		/*if(filter_->update(nc))
		{
			Q_EMIT dataChanged(server_);
			return;
		}*/
	}

	Q_EMIT dataChanged(server_,nc);
}

//==========================================
//
// NodeModelDataHandler
//
//==========================================

NodeModelDataHandler::NodeModelDataHandler(NodeFilterDef *filterDef) :
		servers_(0),
		filterDef_(filterDef)
{
	connect(filterDef_,SIGNAL(changed()),
			this,SLOT(slotFilterDefChanged()));
}

void NodeModelDataHandler::reset(ServerFilter* servers)
{
	clear();
	servers_=servers;

	init();
}

NodeModelData* NodeModelDataHandler::data(int n) const
{
	return (n >=0 && n < data_.size())?data_.at(n):0;
}

ServerHandler* NodeModelDataHandler::server(int n) const
{
	return (n >=0 && n < data_.size())?data_.at(n)->server_:0;
}

ServerHandler* NodeModelDataHandler::server(void* idPointer) const
{
	for(unsigned int i=0; i < data_.size(); i++)
		if(data_.at(i)->server_ == idPointer)
			return data_.at(i)->server_;

	return NULL;
}

void NodeModelDataHandler::add(ServerHandler *server)
{
	//We has to observe the nodes of the server.
	//server->addNodeObserver(this);
	NodeModelData* d=makeData(server);

	connect(d,SIGNAL(dataChanged(ServerHandler*)),
			this,SIGNAL(dataChanged(ServerHandler*)));

	connect(d,SIGNAL(dataChanged(ServerHandler*,Node*)),
			  this,SIGNAL(dataChanged(ServerHandler*,Node*)));

	data_.push_back(d);
}

int NodeModelDataHandler::indexOf(ServerHandler* s) const
{
	for(unsigned int i=0; i < data_.size(); i++)
		if(data_.at(i)->server_ == s)
			return i;
	return -1;
}

//This has to be very fast!!!
bool NodeModelDataHandler::isFiltered(Node *node) const
{
	ServerHandler* server=ServerHandler::find(node);
	int id=indexOf(server);
	if(id != -1 && data_.at(id)->filter_)
	{
		return data_.at(id)->filter_->isFiltered(node);
	}

	return true;
}

int NodeModelDataHandler::numOfNodes(int index) const
{
	if(NodeModelData *d=data(index))
	{
		return d->nodeNum();
	}
	return 0;
}

Node* NodeModelDataHandler::getNodeFromFilter(int totalRow)
{
	int cnt=0;

	for(unsigned int i=0; i < data_.size(); i++)
	{
		NodeFilter *filter=data_.at(i)->filter_;
		if(totalRow-cnt < filter->matchCount())
		{
			return filter->match(totalRow-cnt);
		}
		cnt+=filter->matchCount();
	}

	return NULL;
}

int NodeModelDataHandler::numOfFiltered(int index) const
{
	if(NodeModelData *d=data(index))
	{
		if(d->filter_)
			return d->filter_->matchCount();
	}
	return 0;
}



void NodeModelDataHandler::init()
{
	servers_->addObserver(this);

	for(unsigned int i=0; i < servers_->items().size(); i++)
	{
		if(ServerHandler *server=servers_->items().at(i)->serverHandler())
		{
			add(server);
		}
	}
}

void NodeModelDataHandler::clear()
{
	if(servers_)
		servers_->removeObserver(this);

	for(int i=0; i < data_.size(); i++)
	{
		delete data_.at(i);
	}

	data_.clear();
}

void NodeModelDataHandler::reload()
{
	clear();
	init();
	//resetStateFilter(false); //do not emit change signal
}

void NodeModelDataHandler::runFilter(bool broadcast)
{
	for(unsigned int i=0; i < data_.size(); i++)
	{
		data_.at(i)->runFilter();
	}

	if(broadcast)
	{
		Q_EMIT filterChanged();
	}
}

//ServerFilter observer methods

void NodeModelDataHandler::notifyServerFilterAdded(ServerItem* item)
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

void NodeModelDataHandler::notifyServerFilterRemoved(ServerItem* item)
{
	if(!item)
		return;

	int i=0;
	for(std::vector<NodeModelData*>::iterator it=data_.begin(); it!= data_.end(); it++)
	{
		if((*it)->server_ == item->serverHandler())
		{
			//Notifies the model that a change will happen
			Q_EMIT serverRemoveBegin(i);

			delete *it;
			data_.erase(it);

			//Notifies the model that the change has finished
			Q_EMIT serverRemoveEnd();
			return;
		}
		i++;
	}
}

void NodeModelDataHandler::notifyServerFilterChanged(ServerItem* item)
{
	//Q_EMIT dataChanged();
}

void NodeModelDataHandler::slotFilterDefChanged()
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





NodeModelData* TreeNodeModelDataHandler::makeData(ServerHandler* s)
{
	return new NodeModelData(s,new TreeNodeFilter(filterDef_));
}

NodeModelData* TableNodeModelDataHandler::makeData(ServerHandler* s)
{
	return new NodeModelData(s,new TableNodeFilter(filterDef_));
}



