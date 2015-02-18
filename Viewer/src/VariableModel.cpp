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
#include "VariableModelData.hpp"

//=======================================================================
//
// VariabletModel
//
//=======================================================================

VariableModel::VariableModel(VariableModelDataHandler* data,QObject *parent) :
          QAbstractItemModel(parent),
          data_(data)
{
	connect(data_,SIGNAL(reloadBegin()),
			this,SLOT(slotReloadBegin()));

	connect(data_,SIGNAL(reloadEnd()),
					this,SLOT(slotReloadEnd()));

}

bool VariableModel::hasData() const
{
	return (data_->count()  > 0);
}

int VariableModel::columnCount( const QModelIndex& /*parent */ ) const
{
   	 return 2;
}

int VariableModel::rowCount( const QModelIndex& parent) const
{

	//Parent is the root: the item must be a node or a server
	if(!parent.isValid())
	{
		return data_->count();
	}
	//The parent is a server or a node
	else if(!isVariable(parent))
	{
		int row=parent.row();
		return data_->varNum(row);
	}

	//parent is a variable
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
	if( !index.isValid())
    {
		return QVariant();
	}

	//Data lookup can be costly so we immediately return a default value for all
	//the cases where the default should be used.
	if(role != Qt::DisplayRole && role != Qt::ToolTipRole && role != Qt::DecorationRole && role != Qt::BackgroundRole && role != Qt::ForegroundRole)
	{
		return QVariant();
	}
	//qDebug() << "data" << index << role;

	int row=index.row();
	int level=indexToLevel(index);

	//Server or node
	if(level == 1)
	{
		VariableModelData *d=data_->data(row);
		if(!d)
		{
			return QVariant();
		}

		if(index.column() == 0)
		{
			if(role == Qt::DisplayRole)
			{
				return QString::fromStdString(d->name());
			}
		}

		return QVariant();
	}

	//Variables
	else if (level == 2)
	{
		VariableModelData *d=data_->data(index.parent().row());
		if(!d)
		{
			return QVariant();
		}

		//Generated variable
		if(d->isGenVar(row))
		{
			if(role == Qt::ForegroundRole)
					return QColor(70,70,70);
		}

		if(index.column() == 0)
		{
			return QString::fromStdString(d->name(row));
		}
		else if(index.column() == 1)
		{
			return QString::fromStdString(d->value(row));
		}
		return QVariant();
	}

	return QVariant();
}

bool VariableModel::data(const QModelIndex& idx, QString& name,QString& value) const
{
	QModelIndex idx0=index(idx.row(),0,idx.parent());
	QModelIndex idx1=index(idx.row(),1,idx.parent());

	name=data(idx0,Qt::DisplayRole).toString();
	value=data(idx1,Qt::DisplayRole).toString();
	return true;
}

bool VariableModel::setData(const QModelIndex& index, QString name,QString value)
{
	return false;
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

bool VariableModel::isVariable(const QModelIndex & index) const
{
	return (indexToLevel(index) == 2);
}

/*
bool VariableModel::isServer(const QModelIndex & index) const
{
	if(index.isValid())
	{
		if(indexToLevel(index) == 1 && index.row() == nodes_.size())
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
		if(indexToLevel(index) == 1 && index.row() < nodes_.size())
			return true;
	}
	return false;
}

Node* VariableModel::indexToNode( const QModelIndex & index) const
{
	return (isNode(index))?nodes_.at(index.row()):NULL;
}

QModelIndex VariableModel::nodeToIndex(Node *node) const
{
	for(unsigned int i=0; i < nodes_.size(); i++)
	{
		if(nodes_.at(i) == node)
		{
			return index(i,0);
		}
	}

	return QModelIndex();
}
*/

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

void VariableModel::nodeChanged(const Node* node, const std::vector<ecf::Aspect::Type>&)
{
	/*Node* n=const_cast<Node*>(node);
	QModelIndex idx=nodeToIndex(n);

	Q_EMIT dataChanged(idx,idx);*/
}

void VariableModel::slotReloadBegin()
{
	beginResetModel();
}

void VariableModel::slotReloadEnd()
{
	endResetModel();
}

//=======================================================================
//
// VariableSortModel
//
//=======================================================================

VariableSortModel::VariableSortModel(VariableModel *varModel,QObject* parent) :
	QSortFilterProxyModel(parent),
	varModel_(varModel)
{
	QSortFilterProxyModel::setSourceModel(varModel_);
	setDynamicSortFilter(true);
}


bool VariableSortModel::lessThan(const QModelIndex &sourceLeft, const QModelIndex &sourceRight) const
{
	//Node or server. Here we want the nodes and server to stay unsorted. That is the order should stay as
	//it is defined in the data handler: the selected node stays on top and its ancestors and the server
	//follow each other downwards. This order is reflected in the row index of these items in
	//the varModel: the selected node's row is 0, its parent's row is 1, etc.
	if(!varModel_->isVariable(sourceLeft))
	{
		if(sortOrder() == Qt::AscendingOrder)
			return (sourceLeft.row() < sourceRight.row());
		else
			return (sourceLeft.row() > sourceRight.row());
	}
	//For variables we simply sort according to the string
	else
	{
			return varModel_->data(sourceLeft,Qt::DisplayRole).toString() < varModel_->data(sourceRight,Qt::DisplayRole).toString();
	}
	return true;
}

bool VariableSortModel::filterAcceptsRow(int,const QModelIndex &) const
{
	return true;
}
