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

#include "ServerHandler.hpp"
#include "ViewConfig.hpp"

TreeNodeModel::TreeNodeModel(QObject *parent) : QAbstractItemModel(parent)
{
	for(unsigned int i=0; i < ServerHandler::servers().size(); i++)
	{
			servers_ << ServerHandler::servers().at(i);
	}
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
}


/*void TreeNodeModel::setRootNode(const std::string &server,serverMvQOgcNode *root)
{
	root_=root;

	//Reset the model (views will be notified)
	endResetModel();
}*/


int TreeNodeModel::columnCount( const QModelIndex& /*parent */ ) const
{
   	 return 3;
}

int TreeNodeModel::rowCount( const QModelIndex& parent) const
{
	//qDebug() << "<< rowCount" << parent;


	if(!hasData())
	{
		return 0;
	}
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
	else if(isServer(parent))
	{
			if(ServerHandler *server=indexToServer(parent))
			{
				//defs_ptr defs = server->defs();
				return server->suiteNum();
			}
	}
	else if(Node* parentNode=nodeFromIndex(parent))
	{
		std::vector<node_ptr> nodes;
		parentNode->immediateChildren(nodes);
		//qDebug() << "rowCount" << parent << nodes.size();

		return static_cast<int>(nodes.size());
	}

	return 0;

}


QVariant TreeNodeModel::data( const QModelIndex& index, int role ) const
{
	//Data lookup can be costly so we immediately return a default value for all
	//the cases where the default should be used.
	if( !index.isValid() ||
	   (role != Qt::DisplayRole && role != Qt::ToolTipRole && role != Qt::BackgroundRole))
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
	if(index.column() == 0)
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
	Node* node=nodeFromIndex(index);
	if(!node)
		return QVariant();

	if(role == Qt::DisplayRole)
	{
		switch(index.column())
		{
		case 0: return QString::fromStdString(node->name());
		case 1: return ViewConfig::Instance()->stateName(node->dstate());
		case 2: return QString::fromStdString(node->absNodePath());
		default: return QVariant();
		}
	}
	else if(role == Qt::BackgroundRole)
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
			return createIndex(row,column,row+1);
		else
			return QModelIndex();
	}

	//We are under one of the servers
	else
	{
		node_ptr childNode;

		//The parent is a server: this is a suite
		if(ServerHandler* server=indexToServer(parent))
		{
				//For suite nodes we store the
				defs_ptr defs = server->defs();
			    const std::vector<suite_ptr> &suites = defs->suiteVec();
			    return createIndex(row,column,suites.at(row).get());
		}
		//Non suite nodes
		else
		{
				Node* parentNode=nodeFromIndex(parent);
				std::vector<node_ptr> nodes;
				parentNode->immediateChildren(nodes);
				if(static_cast<int>(nodes.size()) > row)
					return createIndex(row,column,nodes.at(row).get());
		}
	}

	return QModelIndex();

}

/*
   virtual Task* isTask() const   { return NULL;}
   virtual Alias* isAlias() const { return NULL;}
   virtual Submittable* isSubmittable() const { return NULL;}
   virtual NodeContainer* isNodeContainer() const { return NULL;}
   virtual Family* isFamily() const { return NULL;}
   virtual Suite* isSuite() const  { return NULL;}
*/

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


Node* TreeNodeModel::nodeFromIndex( const QModelIndex & index) const
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

QModelIndex TreeNodeModel::parent(const QModelIndex &child) const
{
	//If the child is a server the parent is the root
	if(isServer(child))
		return QModelIndex();

	//Get the parent node
	Node* node=nodeFromIndex(child);
	if(!node)
		return QModelIndex();

	Node *parentNode=node->parent();

	//If there is no parent node it is a suite so its parent is a server
	if(!parentNode)
	{
		//We need to find out which server is the parent
		for(int i=0; i < servers_.count(); i++)
		{
			defs_ptr defs = servers_.at(i)->defs();
			const std::vector<suite_ptr> &suites = defs->suiteVec();
			for(int j=0; j < suites.size(); j++)
			{
					if(suites[j].get() == node)
						return createIndex(i,0,i+1);
			}
		}
		return QModelIndex();
	}
	//else it is a non-suite node so its parent must be another node
	else
	{
		//Node *grandParentNode=parentNode->parent();
		//if(!grandParentNode)
		//	return QModelIndex();

		size_t pos=parentNode->position();
		if(pos != std::numeric_limits<std::size_t>::max())
			return createIndex(pos,0,parentNode);

	}

	return QModelIndex();

		/*Node *grandParentNode=parentNode->parent();
		if(!grandParentNode)
			return QModelIndex();



		std::vector<node_ptr> nodes;
		grandParentNode->immediateChildren(nodes);
		for(size_t i=0; i < nodes.count(); i++)
				if(parentNode==childNode=nodes.at(i))
						return createIndex(node->position(),0,parentNode);*/


}

QModelIndex TreeNodeModel::indexFromNode(Node* node) const
{
	/*if(node != 0 && node->parent() != 0)
	{
		int row=node->parent()->children().indexOf(node);
		if(row != -1)
		{
			return createIndex(row,0,node);
		}
	}*/

	return QModelIndex();
}


