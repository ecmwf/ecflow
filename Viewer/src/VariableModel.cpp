//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "VariableModel.hpp"

#include <QColor>
#include <QDebug>

#include "ServerHandler.hpp"

VariableModel::VariableModel(QObject *parent) :
          QAbstractItemModel(parent),
          server_(0)
{
}

bool VariableModel::hasData() const
{
	return (nodes_.size()  >0 || server_ != NULL);
}

void VariableModel::setData(ViewNodeInfo_ptr data)
{
	beginResetModel();

	nodes_.clear();
	server_=0;

	if(data.get() != 0 && data->isNode())
	{
		Node* n=data->node();
		while(n)
		{
			qDebug() << "name" << QString::fromStdString(n->name());
			nodes_ << n;
			n=n->parent();
		}

		server_=data->server();

	}

	//Reset the model (views will be notified)
	endResetModel();

}

int VariableModel::columnCount( const QModelIndex& /*parent */ ) const
{
   	 return 2;
}

int VariableModel::rowCount( const QModelIndex& parent) const
{
	//There is no data
	if(!hasData())
	{
		return 0;
	}
	//We use only column 0
	//else if(parent.column() > 0)
	//{
	//	return 0;
	//}
	//Parent is the root: the item must be a node or a server
	else if(!parent.isValid())
	{
		return nodes_.count()+((server_)?1:0);
	}
	//The parent is a server
	else if(isServer(parent))
	{
		if(server_)
		{
			ServerDefsAccess defsAccess(server_);  // will reliquish its resources on destruction
			return static_cast<int>(defsAccess.defs()->server().server_variables().size());
		}
	}
	//The parent is a node
	else if(isNode(parent))
	{
		if(Node* parentNode=indexToNode(parent))
		{
				int num=static_cast<int>(parentNode->variables().size());
				std::vector<Variable> v;
				parentNode->gen_variables(v);
				num+=static_cast<int>(v.size());
				return num;
		}
	}

	return 0;
}

Qt::ItemFlags VariableModel::flags ( const QModelIndex & index) const
{
	Qt::ItemFlags defaultFlags;

	defaultFlags=Qt::ItemIsEnabled | Qt::ItemIsSelectable;

	return defaultFlags;
}

QVariant VariableModel::data( const QModelIndex& index, int role ) const
{
	//Data lookup can be costly so we immediately return a default value for all
	//the cases where the default should be used.
	if( !index.isValid())
    {
		return QVariant();
	}

	//qDebug() << "data" << index << role;

	int level=indexToLevel(index);

	//Server  or node
	if(level == 1)
	{
		if(role != Qt::DisplayRole && role != Qt::ToolTipRole)
		{
			return QVariant();
		}

		if(index.column() == 0)
		{
			if(isServer(index))
			{
				return serverData(index,role);
			}
			else if(isNode(index))
			{
				return nodeData(index,role);
			}
		}
		return QVariant();
	}

	//Variables
	else if (level == 2)
	{
		if(role != Qt::DisplayRole && role != Qt::ToolTipRole && role != Qt::ForegroundRole)
		{
				return QVariant();
		}

		QModelIndex p=parent(index);
		if(isServer(p))
		{
			return variableData(server_,index,role);
		}
		else if(Node *node=indexToNode(p))
		{
			return variableData(node,index,role);
		}
	}

	return QVariant();
}

QVariant VariableModel::serverData(const QModelIndex& index,int role) const
{
	if(index.column() == 0 && role == Qt::DisplayRole)
	{
		if(server_)
		{
			return QString::fromStdString(server_->longName());
		}
	}
	return QVariant();
}

QVariant VariableModel::nodeData(const QModelIndex& index, int role) const
{
	if(index.column() == 0 && role == Qt::DisplayRole)
	{
		if(Node* node=indexToNode(index))
		{
			return QString::fromStdString(node->name());
		}
	}
	return QVariant();
}

QVariant VariableModel::variableData(Node* node,const QModelIndex& index, int role) const
{
	if(!node || (role != Qt::DisplayRole && role != Qt::ForegroundRole))
			return QVariant();

	const std::vector<Variable> v=node->variables();

	if(index.row() >=0 && index.row() < v.size())
	{
			if(role == Qt::ForegroundRole)
					return QVariant();

			if(index.column() == 0)
			{
				return QString::fromStdString(v.at(index.row()).name());
			}
			else if(index.column() == 1)
			{
				return QString::fromStdString(v.at(index.row()).theValue());
			}
	}
	else
	{
			if(role == Qt::ForegroundRole)
					return QColor(70,70,70);

			int row=index.row()-v.size();
			std::vector<Variable> genV;
			node->gen_variables(genV);
			if(row >=0 && row < genV.size())
			{
				if(index.column() == 0)
				{
					return QString::fromStdString(genV.at(row).name());
				}
				else if(index.column() == 1)
				{
					return QString::fromStdString(genV.at(row).theValue());
				}
			}
	}

	return QVariant();
}

QVariant VariableModel::variableData(ServerHandler *server,const QModelIndex& index, int role) const
{
	if(!server || role != Qt::DisplayRole)
			return QVariant();

	ServerDefsAccess defsAccess(server);  // will reliquish its resources on destruction
	const std::vector<Variable>& v=defsAccess.defs()->server().server_variables();
	if(index.row() >=0 && index.row() < v.size())
	{
		if(index.column() == 0)
		{
			return QString::fromStdString(v.at(index.row()).name());
		}
		else if(index.column() == 1)
		{
			return QString::fromStdString(v.at(index.row()).theValue());
		}
	}

	return QVariant();
}

QVariant VariableModel::headerData( const int section, const Qt::Orientation orient , const int role ) const
{
	if ( orient != Qt::Horizontal || role != Qt::DisplayRole )
      		  return QAbstractItemModel::headerData( section, orient, role );

   	switch ( section )
	{
   	case 0: return tr("Name");
   	case 1: return tr("Value");
   	default: return QVariant();
   	}

    return QVariant();
}

QModelIndex VariableModel::index( int row, int column, const QModelIndex & parent ) const
{
	if(!hasData() || row < 0 || column < 0)
	{
		return QModelIndex();
	}

	//When parent is the root this index refers to a node or server
	if(!parent.isValid())
	{
		return createIndex(row,column,0);
	}

	//We are under one of the nodes
	else
	{
		return createIndex(row,column,(parent.row()+1)*1000);
	}

	return QModelIndex();

}

QModelIndex VariableModel::parent(const QModelIndex &child) const
{
	if(!child.isValid())
		return QModelIndex();

	int level=indexToLevel(child);
	if(level == 1)
			return QModelIndex();
	else if(level == 2)
	{
		int id=child.internalId();
		int r=id/1000-1;
		return createIndex(r,child.column(),0);
	}

	return QModelIndex();
}

//----------------------------------------------
//
// Server to index mapping and lookup
//
//----------------------------------------------

int VariableModel::indexToLevel(const QModelIndex& index) const
{
	if(!index.isValid())
		return 0;

	int id=index.internalId();
	if(id >=0 && id < 1000)
	{
			return 1;
	}
	return 2;
}

//----------------------------------------------
//
// Server to index mapping and lookup
//
//----------------------------------------------

bool VariableModel::isServer(const QModelIndex & index) const
{
	if(index.isValid())
	{
		if(indexToLevel(index) == 1 && index.row() == nodes_.count())
			return true;
	}
	return false;
}

ServerHandler* VariableModel::indexToServer(const QModelIndex & index) const
{
	return (isServer(index))?server_:NULL;
}

bool VariableModel::isNode(const QModelIndex & index) const
{
	if(index.isValid())
	{
		if(indexToLevel(index) == 1 && index.row() < nodes_.count())
			return true;
	}
	return false;
}

Node* VariableModel::indexToNode( const QModelIndex & index) const
{
	return (isNode(index))?nodes_.at(index.row()):NULL;
}


/*
QModelIndex VariableModel::serverToIndex(ServerHandler* server) const
{
	//For servers the internal id is set to their position in servers_ + 1
	int i;
	if((i=servers_.indexOf(server))!= -1)
			return createIndex(i,0,i+1);

	return QModelIndex();
}

Node* VariableModel::indexToNode( const QModelIndex & index) const
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
*/


