//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "TableNodeModel.hpp"

#include <QDebug>

#include "ChangeMgrSingleton.hpp"

#include "NodeModelData.hpp"
#include "ServerHandler.hpp"
#include "VFilter.hpp"
#include "VNState.hpp"

//=======================================================
//
// TableNodeModel
//
//=======================================================

TableNodeModel::TableNodeModel(VConfig* config,QObject *parent) :
	AbstractNodeModel(config,parent)
{
}

int TableNodeModel::columnCount( const QModelIndex& /*parent */ ) const
{
   	 return 3;
}

int TableNodeModel::rowCount( const QModelIndex& parent) const
{
	//There are no servers
	if(!hasData())
	{
		return 0;
	}
	//We use only column 0
	else if(parent.column() > 0)
	{
		return 0;
	}
	//
	else if(!parent.isValid())
	{
		int cnt=0;
		for(int i=0; i < servers_->count(); i++)
		{
			cnt+=servers_->numOfFiltered(i);
		}

		return cnt;
	}

	return 0;
}


QVariant TableNodeModel::data( const QModelIndex& index, int role ) const
{
	//Data lookup can be costly so we immediately return a default value for all
	//the cases where the default should be used.
	if( !index.isValid() ||
	   (role != Qt::DisplayRole && role != Qt::ToolTipRole && role != Qt::BackgroundRole && role != FilterRole))
    {
		return QVariant();
	}

	//qDebug() << "data" << index << role;

	//Server
	/*if(isServer(index))
	{
		return serverData(index,role);
	}*/

	return nodeData(index,role);
}

QVariant TableNodeModel::serverData(const QModelIndex& index,int role) const
{
	if(role == FilterRole)
			return true;

	else if(index.column() == 0)
	{
		if(ServerHandler *server=indexToServer(index))
		{
			if(role == Qt::DisplayRole)
			{
					return QString::fromStdString(server->longName());
			}
		}
	}
	return QVariant();
}

QVariant TableNodeModel::nodeData(const QModelIndex& index, int role) const
{
	if(role == Qt::BackgroundRole && index.column() != 1)
		return QVariant();

	Node* node=indexToNode(index);
	if(!node)
		return QVariant();

	if(role == Qt::DisplayRole)
	{
		switch(index.column())
		{
		case 0: return QString::fromStdString(node->absNodePath());
		case 1: return VNState::toName(node);
		case 2:
		{
				ServerHandler* s=ServerHandler::find(node);
				return (s)?QString::fromStdString(s->name()):QString();
		}
		default: return QVariant();
		}
	}
	else if(role == Qt::BackgroundRole)
	{
		return VNState::toColour(node);
	}
	else if(role == FilterRole)
		//return static_cast<int>(node->dstate());
		return true;

	return QVariant();
}

QVariant TableNodeModel::headerData( const int section, const Qt::Orientation orient , const int role ) const
{
	if ( orient != Qt::Horizontal || role != Qt::DisplayRole )
      		  return QAbstractItemModel::headerData( section, orient, role );

   	switch ( section )
	{
   	case 0: return tr("Node");
   	case 1: return tr("Status");
   	case 2: return tr("Server");

   	default: return QVariant();
   	}

    return QVariant();
}


QModelIndex TableNodeModel::index( int row, int column, const QModelIndex & parent ) const
{
	if(!hasData() || row < 0 || column < 0)
	{
		return QModelIndex();
	}

	//qDebug() << "index" << row << column << parent;

	if(Node *node=servers_->getNodeFromFilter(row))
	{
		return createIndex(row,column,node);
	}

	/*int cnt=0;
	ServerHandler *server=0;
	for(int i=0; i < servers_->count(); i++)
	{
		NodeFilter *filterservers_->filter(i));
		if(row-cnt < filter->resultCount())
		{
			server=servers_->server(i);
			break;
		}
		cnt+=filter->resultCount());
	}

	if(server)
	{
		if(Node *node=server->findNode(row))
				return createIndex(row,column,node);
	}*/

	return QModelIndex();

	/*//When parent is the root this index refers to a server
	if(!parent.isValid())
	{
		//For the server the internalId is its row index + 1
		if(row < servers_.count())
		{
			return createIndex(row,column,row+1);
		}
		else
			return QModelIndex();
	}

	//We are under one of the servers
	else
	{
		node_ptr childNode;

		//The parent is a server
		if(ServerHandler* server=indexToServer(parent))
		{
			//If there is a rootnode for the server
			if(Node *rn=rootNode(server))
			{
				//qDebug() << "  -->rootNode" << rn->absNodePath().c_str();
				return createIndex(row,column,rn);
			}
			//There is no root node: we show the whole tree for the server.
			//So this item must be a suite!
			else
			{
				if(Node *node=server->findNode(row))
					return createIndex(row,column,node);
			}
		}
		//Parent is not the server: the parent must be another node
		else
		{
			return QModelIndex();
		}
	}

	return QModelIndex();*/

}

QModelIndex TableNodeModel::parent(const QModelIndex &child) const
{
	//Parent is always the root!!!
	return QModelIndex();


	//If the child is a server the parent is the root
	if(isServer(child))
		return QModelIndex();

	//Get the node
	Node* node=indexToNode(child);
	if(!node)
		return QModelIndex();

	//Check if it is a rootNode
	/*QMapIterator<ServerHandler*, Node*> it(rootNodes_);
	while(it.hasNext())
	{
		it.next();

		//For a rootnode the parent is the server
		if(it.value() == node)
		{
			return serverToIndex(it.key());
		}
	}*/

	//The node is not a rootnode

	//Get the parent node
	return serverToIndex(ServerHandler::find(node));

	Node *parentNode=node->parent();

	/*//If there is no parent node it is a suite so its parent is a server
	if(!parentNode)
	{
		return serverToIndex(ServerHandler::find(node));
	}
	//else it is a non-suite node so its parent must be another node
	else
	{
		return QModelIndex();
	}*/

	return QModelIndex();
}

//----------------------------------------------
//
// Server to index mapping and lookup
//
//----------------------------------------------

bool TableNodeModel::isServer(const QModelIndex & index) const
{
	//For servers the internal id is set to their position in servers_ + 1
	if(index.isValid())
	{
		int id=index.internalId()-1;
		return (id >=0 && id < servers_->count());
	}
	return false;
}


ServerHandler* TableNodeModel::indexToServer(const QModelIndex & index) const
{
	//For servers the internal id is set to their position in servers_ + 1
	if(index.isValid())
	{
		int id=index.internalId()-1;
		return servers_->server(id);
	}
	return NULL;
}

QModelIndex TableNodeModel::serverToIndex(ServerHandler* server) const
{
	//For servers the internal id is set to their position in servers_ + 1
	int i;
	if((i=servers_->indexOf(server))!= -1)
			return createIndex(i,0,i+1);

	return QModelIndex();
}

Node* TableNodeModel::indexToNode( const QModelIndex & index) const
{
	//return NULL;

	if(index.isValid())
	{
		//if(!isServer(index))
		//{
			return static_cast<Node*>(index.internalPointer());
		//}

	}
	return NULL;
}

QModelIndex TableNodeModel::nodeToIndex(Node* node, int column) const
{
	return QModelIndex();

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

	return QModelIndex();
}

NodeFilter* TableNodeModel::makeFilter()
{
	return new TableNodeFilter();
}

void TableNodeModel::resetStateFilter(bool broadcast)
{
	beginResetModel();
	servers_->filter(config_->stateFilter());
	endResetModel();

}

