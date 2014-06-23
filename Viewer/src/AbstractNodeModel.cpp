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
#include "ViewConfig.hpp"
#include "ViewFilter.hpp"

AbstractNodeModel::AbstractNodeModel(ServerFilter* serverFilter,QObject *parent) :
   QAbstractItemModel(parent),
   serverFilter_(serverFilter)
{
	serverFilter_->addObserver(this);
	init();
}

AbstractNodeModel::~AbstractNodeModel()
{
	clean();
}

void AbstractNodeModel::notifyServerFilterChanged()
{
	reload();
}

void AbstractNodeModel::init()
{
	for(unsigned int i=0; i < serverFilter_->servers().size(); i++)
	{
				ServerHandler *server=ServerHandler::find(serverFilter_->servers().at(i)->host(),
						serverFilter_->servers().at(i)->port());

				server->addNodeObserver(this);

				servers_ << server;
				rootNodes_[server] = NULL;
	}
}

void AbstractNodeModel::clean()
{
	foreach(ServerHandler* s,servers_)
	{
		s->removeNodeObserver(this);
	}

	servers_.clear();
	rootNodes_.clear();
}

void AbstractNodeModel::reload()
{
	beginResetModel();
	clean();
	init();
	endResetModel();
}


bool AbstractNodeModel::hasData() const
{
	return servers_.size() >0;
}

void AbstractNodeModel::dataIsAboutToChange()
{
	beginResetModel();
}

void AbstractNodeModel::addServer(ServerHandler *server)
{
	servers_ << server;
	rootNodes_[servers_.back()] = NULL;
}


Node * AbstractNodeModel::rootNode(ServerHandler* server) const
{
	QMap<ServerHandler*,Node*>::const_iterator it=rootNodes_.find(server);
	if(it != rootNodes_.end())
		return it.value();
	return NULL;
}


void AbstractNodeModel::setRootNode(Node *node)
{
	if(ServerHandler *server=ServerHandler::find(node))
	{
		beginResetModel();

		rootNodes_[server]=node;

		//Reset the model (views will be notified)
		endResetModel();

		qDebug() << "setRootNode finished";
	}
}

//----------------------------------------------
//
// Server to index mapping and lookup
//
//----------------------------------------------

bool AbstractNodeModel::isServer(const QModelIndex & index) const
{
	//For servers the internal id is set to their position in servers_ + 1
	if(index.isValid())
	{
		int id=index.internalId()-1;
		return (id >=0 && id < servers_.count());
	}
	return false;
}


ServerHandler* AbstractNodeModel::indexToServer(const QModelIndex & index) const
{
	//For servers the internal id is set to their position in servers_ + 1
	if(index.isValid())
	{
		int id=index.internalId()-1;
		if(id >=0 && id < servers_.count())
				return servers_.at(id);
	}
	return NULL;
}

QModelIndex AbstractNodeModel::serverToIndex(ServerHandler* server) const
{
	//For servers the internal id is set to their position in servers_ + 1
	int i;
	if((i=servers_.indexOf(server))!= -1)
			return createIndex(i,0,i+1);

	return QModelIndex();
}

Node* AbstractNodeModel::indexToNode( const QModelIndex & index) const
{
	if(index.isValid())
	{
		if(!isServer(index))
		{
			return static_cast<Node*>(index.internalPointer());
		}

	}
	return NULL;
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


QModelIndex AbstractNodeModel::nodeToIndex(Node* node, int column) const
{
	if(!node)
		return QModelIndex();

	if(node->parent() != 0)
	{
		int row=ServerHandler::indexOfImmediateChild(node);
		if(row != -1)
		{
					return createIndex(row,column,node);
		}
	}
	else
	{
			if(ServerHandler* server=ServerHandler::find(node))
			{
				int row=server->indexOfSuite(node);
				if(row != -1)
						return createIndex(row,column,node);
			}
	}

	return QModelIndex();
}

void AbstractNodeModel::slotUpdateNode(const Node* node, const std::vector<ecf::Aspect::Type>& types)
{
	if(node==NULL)
		return;

	qDebug() << "observer is called" << QString::fromStdString(node->name());
	for(unsigned int i=0; i < types.size(); i++)
		qDebug() << "  type:" << types.at(i);

	Node* nc=const_cast<Node*>(node);

	QModelIndex index1=nodeToIndex(nc,0);
	QModelIndex index2=nodeToIndex(nc,2);

	Node *nd1=indexToNode(index1);
	Node *nd2=indexToNode(index2);

	//qDebug() << "indexes" << index1 << index2;
	//qDebug() << "index pointers " << index1.internalPointer() << index2.internalPointer();
	qDebug() << "    --->" << QString::fromStdString(nd1->name()) << QString::fromStdString(nd2->name());

	emit dataChanged(index1,index2);
}





