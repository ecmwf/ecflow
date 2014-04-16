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

TreeNodeModel::TreeNodeModel(ServerFilter* serverFilter,QObject *parent) :
   QAbstractItemModel(parent),
   serverFilter_(serverFilter)
{
	serverFilter_->addObserver(this);
	init();
}


void TreeNodeModel::notifyServerFilterChanged()
{
	beginResetModel();
	clean();

	for(unsigned int i=0; i < serverFilter_->servers().size(); i++)
	{
			ServerHandler *server=ServerHandler::find(serverFilter_->servers().at(i)->host(),
					serverFilter_->servers().at(i)->port());
			//initObserver(server);

			servers_ << server;
			rootNodes_[server] = NULL;
	}

	endResetModel();
}

void TreeNodeModel::init()
{
	/*for(unsigned int i=0; i < ServerHandler::servers().size(); i++)
	{
			ServerHandler *server=ServerHandler::servers().at(i);
			initObserver(server);

			servers_ << server;
			rootNodes_[server] = NULL;
	}*/

	for(unsigned int i=0; i < serverFilter_->servers().size(); i++)
		{
				ServerHandler *server=ServerHandler::find(serverFilter_->servers().at(i)->host(),
						serverFilter_->servers().at(i)->port());
				initObserver(server);

				servers_ << server;
				rootNodes_[server] = NULL;
		}


}

void TreeNodeModel::initObserver(ServerHandler* server)
{
	ServerDefsAccess defsAccess(server);  // will reliquish its resources on destruction
	defs_ptr d = defsAccess.defs();
	if(d == NULL)
		return;

	const std::vector<suite_ptr> &suites = d->suiteVec();
	for(unsigned int i=0; i < suites.size();i++)
	{
		ChangeMgrSingleton::instance()->attach(suites.at(i).get(),this);

		std::set<Node*> nodes;
		suites.at(i)->allChildren(nodes);
		for(std::set<Node*>::iterator it=nodes.begin(); it != nodes.end(); it++)
			ChangeMgrSingleton::instance()->attach((*it),this);

	}
}

void TreeNodeModel::clean()
{
	servers_.clear();
	rootNodes_.clear();
}

void TreeNodeModel::reload()
{
	beginResetModel();
	clean();
	init();
	endResetModel();
}


bool TreeNodeModel::hasData() const
{
	return servers_.size() >0;
}

void TreeNodeModel::dataIsAboutToChange()
{
	beginResetModel();
}

void TreeNodeModel::addServer(ServerHandler *server)
{
	servers_ << server;
	rootNodes_[servers_.back()] = NULL;
}


Node * TreeNodeModel::rootNode(ServerHandler* server) const
{
	QMap<ServerHandler*,Node*>::const_iterator it=rootNodes_.find(server);
	if(it != rootNodes_.end())
		return it.value();
	return NULL;
}


void TreeNodeModel::setRootNode(Node *node)
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
	//Parent is the root: the item must be a server!
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
			//There is a rootNode for the server
			if(Node *rn=rootNode(server))
			{
				return 1;
			}
			//We show the whole tree for the server
			else
			{
				return server->numSuites();
			}
		}
	}
	//The parent is a node
	else if(Node* parentNode=indexToNode(parent))
	{
		//qDebug() << "  -->node" << ServerHandler::numOfImmediateChildren(parentNode);
		return ServerHandler::numOfImmediateChildren(parentNode);
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

	//qDebug() << "data" << index << role;

	//Server
	if(isServer(index))
	{
		return serverData(index,role);
	}

	return nodeData(index,role);
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
			return static_cast<int>(node->dstate());
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

	//qDebug() << "index" << row << column << parent;

	//When parent is the root this index refers to a server
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
				if(Node *suite=server->suiteAt(row))
				{
					//ChangeMgrSingleton::instance()->attach(suite,this);
					return createIndex(row,column,suite);
				}
				else
					return QModelIndex();
			}
		}
		//Parent is not the server: the parent must be another node
		else
		{
				Node* parentNode=indexToNode(parent);
				if(Node *n=ServerHandler::immediateChildAt(parentNode,row))
				{
					//ChangeMgrSingleton::instance()->attach(n,this);
					return createIndex(row,column,n);

				}
				else
					return QModelIndex();
		}
	}

	return QModelIndex();

}

QModelIndex TreeNodeModel::parent(const QModelIndex &child) const
{
	//If the child is a server the parent is the root
	if(isServer(child))
		return QModelIndex();

	//Get the node
	Node* node=indexToNode(child);
	if(!node)
		return QModelIndex();

	//Check if it is a rootNode
	QMapIterator<ServerHandler*, Node*> it(rootNodes_);
	while(it.hasNext())
	{
		it.next();

		//For a rootnode the parent is the server
		if(it.value() == node)
		{
			return serverToIndex(it.key());
		}
	}

	//The node is not a rootnode

	//Get the parent node
	Node *parentNode=node->parent();

	//If there is no parent node it is a suite so its parent is a server
	if(!parentNode)
	{
		return serverToIndex(ServerHandler::find(node));
	}
	//else it is a non-suite node so its parent must be another node
	else
	{
		size_t pos=parentNode->position();
		if(pos != std::numeric_limits<std::size_t>::max())
			return createIndex(pos,0,parentNode);

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
	//For servers the internal id is set to their position in servers_ + 1
	if(index.isValid())
	{
		int id=index.internalId()-1;
		return (id >=0 && id < servers_.count());
	}
	return false;
}


ServerHandler* TreeNodeModel::indexToServer(const QModelIndex & index) const
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

QModelIndex TreeNodeModel::serverToIndex(ServerHandler* server) const
{
	//For servers the internal id is set to their position in servers_ + 1
	int i;
	if((i=servers_.indexOf(server))!= -1)
			return createIndex(i,0,i+1);

	return QModelIndex();
}

Node* TreeNodeModel::indexToNode( const QModelIndex & index) const
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


ViewNodeInfo_ptr TreeNodeModel::nodeInfo(const QModelIndex& index) const
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


QModelIndex TreeNodeModel::nodeToIndex(Node* node, int column) const
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


void TreeNodeModel::update(const Node* node, const std::vector<ecf::Aspect::Type>& types)
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
	invalidateFilter();
}

bool TreeNodeFilterModel::filterAcceptsRow(int sourceRow,const QModelIndex& sourceParent) const
{
	if(!viewFilter_->isNodeStateFiltered())
		return true;

	QModelIndex index = sourceModel()->index(sourceRow, 1, sourceParent);
	const std::set<DState::State> ns=viewFilter_->nodeState();
	int intSt=sourceModel()->data(index,TreeNodeModel::FilterRole).toInt();
	if(intSt<0)
			return true;
	else
	{
		DState::State st=static_cast<DState::State>(intSt);
		if(ns.find(st) != ns.end())
			return true;
	}
	return false;
}


