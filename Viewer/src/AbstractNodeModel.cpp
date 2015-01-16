//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "AbstractNodeModel.hpp"

#include <QDebug>

#include "ChangeMgrSingleton.hpp"

#include "ServerFilter.hpp"
#include "ServerHandler.hpp"
#include "ServerItem.hpp"
#include "VFilter.hpp"
#include "VNState.hpp"

int NodeModelServerItem::nodeNum() const
{
	if(nodeNum_==-1)
		nodeNum_=server_->numberOfNodes();

	return nodeNum_;
}

bool NodeModelServerItem::isFiltered(Node* node) const
{
	return !nodeFilter_.contains(node);
}

void NodeModelServerItem::resetFilter(VFilter* stateFilter)
{
	nodeFilter_.clear();

	//If all states are visible
	if(stateFilter->isComplete())
		return;

	for(unsigned int j=0; j < server_->numSuites();j++)
	{
		filterState(server_->suiteAt(j),stateFilter);
	}
}

bool NodeModelServerItem::filterState(node_ptr node,VFilter* stateFilter)
{
	bool ok=false;
	if(stateFilter->isSet(VNState::toState(node.get())))
	{
		ok=true;
	}

	std::vector<node_ptr> nodes;
	node->immediateChildren(nodes);

	for(std::vector<node_ptr>::iterator it=nodes.begin(); it != nodes.end(); it++)
	{
		if(filterState(*it,stateFilter) == true && ok == false)
		{
			ok=true;
		}
	}

	if(!ok)
		nodeFilter_ << node.get();

	return ok;
}

//==========================================
//
// NodeModelServers
//
//==========================================

ServerHandler* NodeModelServers::server(int n) const
{
	return (n >=0 && n < items_.count())?items_.at(n).server_:0;
}


ServerHandler* NodeModelServers::server(void* idPointer) const
{
	for(int i=0; i < items_.count(); i++)
		if(items_.at(i).server_ == idPointer)
			return items_.at(i).server_;

	return NULL;
}


int NodeModelServers::index(ServerHandler* s) const
{
	for(int i=0; i < items_.count(); i++)
		if(items_.at(i).server_ == s)
			return i;
	return -1;
}

void NodeModelServers::add(ServerHandler *server)
{
	items_ << NodeModelServerItem(server);
}

void NodeModelServers::resetFilter(VFilter *stateFilter)
{
	Q_FOREACH(NodeModelServerItem item,items_)
	{
		item.resetFilter(stateFilter);
	}
}

void NodeModelServers::clearFilter()
{
	Q_FOREACH(NodeModelServerItem item,items_)
	{
		item.nodeFilter_.clear();
	}
}

void NodeModelServers::nodeFilter(int n,QSet<Node*> s)
{
	if(n >=0 && n < items_.count())
	{
		items_[n].nodeFilter_=s;
	}
}

bool NodeModelServers::isFiltered(Node *node) const
{
	ServerHandler* server=ServerHandler::find(node);
	int id=index(server);
	if(id != -1)
	{
		return items_.at(id).isFiltered(node);
	}

	return true;
}

//=======================================
//
//  AbstractNodeModel
//
//=======================================

AbstractNodeModel::AbstractNodeModel(VConfig * config,QObject *parent) :
   QAbstractItemModel(parent),
   config_(config),
   active_(false)
{
	//Register as VConfig observer. The has to know about any
	//changes in VConfig!
	config_->addObserver(this);

	//At this point the model is empty!!
}

AbstractNodeModel::~AbstractNodeModel()
{
	config_->removeObserver(this);
	clean();
}

void AbstractNodeModel::active(bool active)
{
	if(active_ != active)
	{
		active_=active;

		beginResetModel();

		//When the model becomes active we reload everything
		if(active_)
		{
			init();

			//Initialises the filter
			resetStateFilter(false);
		}

		//When the model becomes inactive we clean it and
		//release all the resources
		else
		{
			clean();
		}

		endResetModel();

		//After finishing reset the view will automatically be notified about
		//the changes. Also the filter model will be notified!
	}
}

//Called when the list of servers to be displayed has changed.
void AbstractNodeModel::notifyConfigChanged(ServerFilter*)
{
	if(active_)
		reload();
}

void AbstractNodeModel::init()
{
	ServerFilter *filter=config_->serverFilter();
	for(unsigned int i=0; i < filter->items().size(); i++)
	{
		if(ServerHandler *server=filter->items().at(i)->serverHandler())
		{
		    //The model has to observe the nodes o the server.
		    server->addNodeObserver(this);

		    //The model stores the servers it has to deal with in a local object.
		    servers_.add(server);
		}
	}
}

void AbstractNodeModel::clean()
{
	for(int i=0; i < servers_.count(); i++)
	{
		servers_.server(i)->removeNodeObserver(this);
	}

	servers_.clear();
}

void AbstractNodeModel::reload()
{
	if(active_)
	{
		beginResetModel();
		clean();
		init();
		resetStateFilter(false); //do not emit change signal
		endResetModel();
	}
}


bool AbstractNodeModel::hasData() const
{
	return servers_.count() >0;
}

void AbstractNodeModel::dataIsAboutToChange()
{
	beginResetModel();
}

//Reset the state filter
void AbstractNodeModel::resetStateFilter(bool broadcast)
{
	servers_.resetFilter(config_->stateFilter());

	//Notify the filter model
	if(broadcast)
		Q_EMIT filterChanged();
}

void AbstractNodeModel::addServer(ServerHandler *server)
{
	//servers_ << server;
	//rootNodes_[servers_.back()] = NULL;
}


Node * AbstractNodeModel::rootNode(ServerHandler* server) const
{
	/*QMap<ServerHandler*,Node*>::const_iterator it=rootNodes_.find(server);
	if(it != rootNodes_.end())
		return it.value();*/
	return NULL;
}


void AbstractNodeModel::setRootNode(Node *node)
{
	/*if(ServerHandler *server=ServerHandler::find(node))
	{
		beginResetModel();

		rootNodes_[server]=node;

		//Reset the model (views will be notified)
		endResetModel();

		qDebug() << "setRootNode finished";
	}*/
}

//----------------------------------------------
//
// Server to index mapping and lookup
//
//----------------------------------------------

QModelIndex AbstractNodeModel::infoToIndex(VInfo_ptr info,int column) const
{
	if(info)
	{
		if(info->isServer())
		{
			if(ServerHandler *s=info->server())
			{
				return serverToIndex(s);
			}
		}
		else if(Node* n=info->node())
		{
			return nodeToIndex(n);
		}
	}

	return QModelIndex();
}

VInfo_ptr AbstractNodeModel::nodeInfo(const QModelIndex& index)
{
	if(!index.isValid())
	{
		VInfo_ptr res;
		return res;
	}

	ServerHandler *server=indexToServer(index);
	if(server)
	{
		VInfo_ptr res(VInfo::make(server));
		return res;
	}
	else
	{
		Node* node=indexToNode(index);
		VInfo_ptr res(VInfo::make(node));
		return res;
	}
}


void AbstractNodeModel::notifyNodeChanged(const Node* node, const std::vector<ecf::Aspect::Type>& types)
{
	if(node==NULL)
		return;

	qDebug() << "observer is called" << QString::fromStdString(node->name());
	/*for(unsigned int i=0; i < types.size(); i++)
		qDebug() << "  type:" << types.at(i);*/

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





