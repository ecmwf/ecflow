//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "TreeNodeModel.hpp"

#include <QDebug>

#include "ChangeMgrSingleton.hpp"

#include "ServerFilter.hpp"
#include "ServerHandler.hpp"
#include "ViewConfig.hpp"
#include "ViewFilter.hpp"


//=======================================
//
// TreeNodeModel
//
//=======================================


TreeNodeModel::TreeNodeModel(ServerFilter* serverFilter,QObject *parent) :
   AbstractNodeModel(serverFilter,parent)
{

}

int TreeNodeModel::columnCount( const QModelIndex& /*parent */ ) const
{
   	 return 3;
}

int TreeNodeModel::rowCount( const QModelIndex& parent) const
{
	//qDebug() << "rowCount" << parent;

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
	//Parent is the root
	else if(!parent.isValid())
	{
		//qDebug() << "rowCount" << parent << servers_.count();
		return servers_.count();
	}
	//The parent is a server
	else if(isServer(parent))
	{
		if(ServerHandler *server=indexToServer(parent))
		{
			//We show the whole tree for the server
			//qDebug() << "  -->suiteNum" <<server->numSuites();
			return server->numSuites();
		}
	}
	//The parent is a node
	else if(Node* parentNode=indexToNode(parent))
	{
		//qDebug() << "  -->node" << parentNode->name().c_str() << ServerHandler::numOfImmediateChildren(parentNode);
		int num=attributesNum(parentNode);
		return num+ServerHandler::numOfImmediateChildren(parentNode);
	}

	return 0;
}

Qt::ItemFlags TreeNodeModel::flags ( const QModelIndex & index) const
{
	Qt::ItemFlags defaultFlags;

	defaultFlags=Qt::ItemIsEnabled | Qt::ItemIsSelectable;

	return defaultFlags;
}

QVariant TreeNodeModel::data( const QModelIndex& index, int role ) const
{
	//Data lookup can be costly so we immediately return a default value for all
	//the cases where the default should be used.
	if( !index.isValid() ||
	   (role != Qt::DisplayRole && role != Qt::ToolTipRole && role != Qt::BackgroundRole && role != FilterRole))
    {
		return QVariant();
	}

	//Server
	int id;
	if(isServer(index))
	{
		return serverData(index,role);
	}
	else if(isNode(index))
		return nodeData(index,role);
	else if(isAttribute(index))
		return QVariant(); //attributesData(index,role);


	return QVariant();
}

QVariant TreeNodeModel::serverData(const QModelIndex& index,int role) const
{
	if(role == FilterRole)
		return -1;

	else if(index.column() == 0 && role == Qt::BackgroundRole)
	{
			return ViewConfig::Instance()->stateColour(DState::UNKNOWN);
	}

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

QVariant TreeNodeModel::nodeData(const QModelIndex& index, int role) const
{
	Node* node=indexToNode(index);
	if(!node)
		return QVariant();

	if(role == Qt::DisplayRole)
	{
		switch(index.column())
		{
		case 0:	 return QString::fromStdString(node->name());
		case 1:  return ViewConfig::Instance()->stateName(node->dstate());
		case 2: return QString::fromStdString(node->absNodePath());
		default: return QVariant();
		}
	}
	else if(role == FilterRole)
	{
		return QVariant(servers_.isFiltered(node));
	}
	else if(index.column() == 0 && role == Qt::BackgroundRole)
	{
		return ViewConfig::Instance()->stateColour(node->dstate());
	}

	return QVariant();
}

QVariant TreeNodeModel::headerData( const int section, const Qt::Orientation orient , const int role ) const
{
	if ( orient != Qt::Horizontal || role != Qt::DisplayRole )
      		  return QAbstractItemModel::headerData( section, orient, role );

   	switch ( section )
	{
   	case 0: return tr("Node");
   	case 1: return tr("Status");
   	case 2: return tr("Path");
   	default: return QVariant();
   	}

    return QVariant();
}

QModelIndex TreeNodeModel::index( int row, int column, const QModelIndex & parent ) const
{
	if(!hasData() || row < 0 || column < 0)
	{
		return QModelIndex();
	}

	//When parent is the root this index refers to a server
	if(!parent.isValid())
	{
		//For the server the internal pointer is NULL
		if(row < servers_.count())
		{
			void* p=NULL;
			return createIndex(row,column,(void*)NULL);
		}
	}

	//Here we are under one of the servers
	else
	{
		//The parent is a server: this index refers to a suite. We set the
		//server as an internal pointer
		if(ServerHandler* server=indexToServer(parent))
		{
			return createIndex(row,column,server);
		}
		//Parent is not the server: the parent must be another node
		else if(Node* parentNode=indexToNode(parent))
		{
			return createIndex(row,column,parentNode);
		}
	}

	return QModelIndex();

}

QModelIndex TreeNodeModel::parent(const QModelIndex &child) const
{
	//If the child is a server the parent is the root
	if(isServer(child))
		return QModelIndex();


	//Child is a suite. Its internal pointer points to a server.
	if(ServerHandler *s=servers_.server(child.internalPointer()))
	{
		//The parent is a server
		int serverIdx=servers_.index(s);
		return createIndex(serverIdx,0,(void*)NULL);
	}

	//Child is a non-suite node or an attribute. Its internal pointer points to the parent node.
	else if(Node *parentNode=static_cast<Node*>(child.internalPointer()))
	{
			//Child is a Node
			if(child.row() >= attributesNum(parentNode))
			{
				//The parent is a suite: its internal pointer is the server
				if(parentNode->isSuite())
				{
					if(ServerHandler* server=ServerHandler::find(parentNode))
					{
						return createIndex(server->indexOfSuite(parentNode),0,server);
					}
				}
				//The parent is a non-suite node: its internal pointer
				//is its parent (i.e.. the grandparent)
				else if(Node *grandParentNode=parentNode->parent())
				{
					size_t pos=parentNode->position();
					if(pos != std::numeric_limits<std::size_t>::max())
						return createIndex(pos,0,grandParentNode);
				}
			}

			//Child is an attribute
			else
			{
				return QModelIndex();

			}
	}

	return QModelIndex();
}

//----------------------------------------------
//
// Server to index mapping and lookup
//
//----------------------------------------------

bool TreeNodeModel::isServer(const QModelIndex & index) const
{
	return (index.isValid() && index.internalPointer() == NULL);
}

bool TreeNodeModel::isNode(const QModelIndex & index) const
{
	return (indexToNode(index) != NULL);
}

bool TreeNodeModel::isAttribute(const QModelIndex & index) const
{
	if(index.isValid())
	{
		//int id=index.internalId()-1;
		//return id > 1000;
	}
	return false;
}

ServerHandler* TreeNodeModel::indexToServer(const QModelIndex & index) const
{
	//For servers the internal id is a nul pointer
	if(index.isValid())
	{
		if(index.internalPointer() == NULL)
			return servers_.server(index.row());
	}

	return NULL;
}

QModelIndex TreeNodeModel::serverToIndex(ServerHandler* server) const
{
	//For servers the internal id is set to their position in servers_ + 1
	int i;
	if((i=servers_.index(server))!= -1)
			return createIndex(i,0,(void*)NULL);

	return QModelIndex();
}

Node* TreeNodeModel::indexToNode( const QModelIndex & index) const
{
	if(index.isValid() && !isServer(index))
	{
		//If suite (the internal pointer points to a server id pointer
		if(ServerHandler *s=servers_.server(index.internalPointer()))
		{
			return s->suiteAt(index.row()).get();
		}
		//Other nodes
		else if(Node *parentNode=static_cast<Node*>(index.internalPointer()))
		{
			if(index.row() >= attributesNum(parentNode))
			{
				return ServerHandler::immediateChildAt(parentNode,index.row());
			}
		}
	}

	return NULL;
}

QModelIndex TreeNodeModel::nodeToIndex(Node* node, int column) const
{
	if(!node)
		return QModelIndex();

	//If the node is a suite
	if(node->isSuite())
	{
		if(ServerHandler* server=ServerHandler::find(node))
		{
				int row=server->indexOfSuite(node);
				if(row != -1)
						return createIndex(row,column,server);
		}
	}

	//Other nodes
	else if(Node *parentNode=node->parent())
	{
		int row=ServerHandler::indexOfImmediateChild(node);
		if(row != -1)
		{
			if(row != -1)
			{
				return createIndex(row,column,parentNode);
			}
		}
	}

	return QModelIndex();
}



void TreeNodeModel::setFilter(const std::set<DState::State>& ns)
{
	servers_.clearFilter();

	for(int i=0; i < servers_.count(); i++)
	{
		ServerHandler *server=servers_.server(i);
		QList<Node*> filterVec;

		for(unsigned int j=0; j < server->numSuites();j++)
		{
			filter(server->suiteAt(j),ns,filterVec);
		}

		servers_.nodeFilter(i,filterVec);

	}
}

bool TreeNodeModel::filter(node_ptr node,const std::set<DState::State>& ns,QList<Node*>& filterVec)
{
	bool ok=false;
	DState::State st=node->dstate();
	if(ns.find(st) != ns.end())
	{
			ok=true;
	}

	std::vector<node_ptr> nodes;
	node->immediateChildren(nodes);

	for(std::vector<node_ptr>::iterator it=nodes.begin(); it != nodes.end(); it++)
	{
		if(filter(*it,ns,filterVec) == true && ok == false)
		{
			ok=true;
		}
	}

	if(!ok)
		filterVec << node.get();

	return ok;
}


int TreeNodeModel::attributesNum(Node *node) const
{
	return 0;

	if(node)
	{
		std::vector<Variable> v;
		node->gen_variables(v);
		return static_cast<int>(v.size());
	}
	return 0;
}



//=======================================
//
// TreeNodeFilterModel
//
//=======================================

TreeNodeFilterModel::TreeNodeFilterModel(ViewFilter* filterData,QObject *parent) :
		QSortFilterProxyModel(parent),
		viewFilter_(filterData)
{
	viewFilter_->addObserver(this);
}

TreeNodeFilterModel::~TreeNodeFilterModel()
{
	viewFilter_->removeObserver(this);
}

void TreeNodeFilterModel::notifyFilterChanged()
{
	if(TreeNodeModel* m=static_cast<TreeNodeModel*>(sourceModel()))
		m->setFilter(viewFilter_->nodeState());

	invalidateFilter();
}

bool TreeNodeFilterModel::filterAcceptsRow(int sourceRow,const QModelIndex& sourceParent) const
{
	if(!viewFilter_->isNodeStateFiltered())
		return true;

	QModelIndex index = sourceModel()->index(sourceRow, 1, sourceParent);
	return sourceModel()->data(index,TreeNodeModel::FilterRole).toBool();
}

