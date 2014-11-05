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

bool NodeModelServerItem::isFiltered(Node* node) const
{
	return !nodeFilter_.contains(node);
}

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

void NodeModelServers::clearFilter()
{
	foreach(NodeModelServerItem item,items_)
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
   config_(config)
{
	config_->addObserver(this);
	init();
}

AbstractNodeModel::~AbstractNodeModel()
{
	config_->removeObserver(this);
	clean();
}

void AbstractNodeModel::notifyConfigChanged(ServerFilter*)
{
	reload();
}

void AbstractNodeModel::init()
{
	ServerFilter *filter=config_->serverFilter();
	for(unsigned int i=0; i < filter->items().size(); i++)
	{
			//ServerHandler *server=ServerHandler::find(filter->items().at(i)->host(),filter->at().at(i)->port());
		    if(ServerHandler *server=filter->items().at(i)->serverHandler())
			{
		    		server->addNodeObserver(this);
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
	beginResetModel();
	clean();
	init();
	resetStateFilter(false); //do not emit change signal
	endResetModel();
}


bool AbstractNodeModel::hasData() const
{
	return servers_.count() >0;
}

void AbstractNodeModel::dataIsAboutToChange()
{
	beginResetModel();
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

QModelIndex AbstractNodeModel::infoToIndex(ViewNodeInfo_ptr info,int column) const
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

ViewNodeInfo_ptr AbstractNodeModel::nodeInfo(const QModelIndex& index) const
{
	if(!index.isValid())
	{
		ViewNodeInfo_ptr res(new ViewNodeInfo());
		return res;
	}

	ServerHandler *server=indexToServer(index);
	if(server)
	{
		ViewNodeInfo_ptr res(new ViewNodeInfo(server));
		return res;
	}
	else
	{
		Node* node=indexToNode(index);
		ViewNodeInfo_ptr res(new ViewNodeInfo(node));
		return res;
	}
}


void AbstractNodeModel::slotNodeChanged(const Node* node, QList<ecf::Aspect::Type> types)
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

	emit dataChanged(index1,index2);
}





