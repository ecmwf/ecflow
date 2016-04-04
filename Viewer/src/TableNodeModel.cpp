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
#include <QMetaMethod>

#include "ModelColumn.hpp"
#include "ServerHandler.hpp"
#include "VFilter.hpp"
#include "VIcon.hpp"
#include "VModelData.hpp"
#include "VNode.hpp"
#include "VNState.hpp"

//=======================================================
//
// TableNodeModel
//
//=======================================================


TableNodeModel::TableNodeModel(ServerFilter* serverFilter,NodeFilterDef* filterDef,QObject *parent) :
	AbstractNodeModel(parent),
	data_(0),
	columns_(0)
{
	columns_=ModelColumn::def("table_columns");

	assert(columns_);

	//Create the data handler for the model.
	data_=new VTableModelData(filterDef,this);

	data_->reset(serverFilter);
}

VModelData* TableNodeModel::data() const
{
	return data_;
}

int TableNodeModel::columnCount( const QModelIndex& /*parent */ ) const
{
   	 return columns_->count();
}

int TableNodeModel::rowCount( const QModelIndex& parent) const
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
	//"parent" is the root
	else if(!parent.isValid())
	{
		//The total number of nodes displayed
		int cnt=0;
		for(int i=0; i < data_->count(); i++)
		{
            if(!data_->server(i)->inScan())
                cnt+=data_->numOfNodes(i);
		}
		//qDebug() << "table count" << cnt;

		return cnt;
	}

	return 0;
}


QVariant TableNodeModel::data( const QModelIndex& index, int role ) const
{
	//Data lookup can be costly so we immediately return a default value for all
	//the cases where the default should be used.
	if( !index.isValid() ||
       (role != Qt::DisplayRole && role != Qt::ToolTipRole && role != Qt::BackgroundRole && role != IconRole))
    {
		return QVariant();
	}

	//We only display nodes!!
	return nodeData(index,role);
}

QVariant TableNodeModel::nodeData(const QModelIndex& index, int role) const
{
	VNode* vnode=indexToNode(index);
	if(!vnode || !vnode->node())
		return QVariant();

	if(role == Qt::DisplayRole)
	{
		QString id=columns_->id(index.column());

		if(id == "path")
			return QString::fromStdString(vnode->absNodePath());
		else if(id == "status")
			return vnode->stateName();
		else if(id == "type")
			return QString::fromStdString(vnode->nodeType());

		//Attributes
		else if(id == "event" || id == "label" || id == "meter" || id == "trigger")
		{
			QStringList lst;
			if(vnode->getAttributeData(id.toStdString(),0,lst))
				return lst;
			else
				return QVariant();
		}

        else if(id == "icon")
            return VIcon::pixmapList(vnode,0);
	}
	else if(role == Qt::BackgroundRole)
	{
		return vnode->stateColour();
	}
	else if(role == IconRole)
	{
		if(columns_->id(index.column()) =="path")
			return VIcon::pixmapList(vnode,0);
		else
			return QVariant();
	}

	return QVariant();
}

QVariant TableNodeModel::headerData( const int section, const Qt::Orientation orient , const int role ) const
{
	if ( orient != Qt::Horizontal || (role != Qt::DisplayRole && role != Qt::UserRole ))
      		  return QAbstractItemModel::headerData( section, orient, role );

	if (section < 0 || section >= columns_->count())  // this can happen during a server reset
			return QVariant();

	if(role == Qt::DisplayRole)
		return columns_->label(section);
	else if(role == Qt::UserRole)
		return columns_->id(section);

	return QVariant();
}


QModelIndex TableNodeModel::index( int row, int column, const QModelIndex & parent ) const
{
	if(!hasData() || row < 0 || column < 0)
	{
		return QModelIndex();
	}

	//qDebug() << "index" << row << column << parent;

    if(VNode *node=data_->getNodeFromFilter(row))
	{
		return createIndex(row,column,node);
	}

	return QModelIndex();
}

QModelIndex TableNodeModel::parent(const QModelIndex &child) const
{
	//Parent is always the root!!!
	return QModelIndex();
}

//----------------------------------------------
//
// Server to index mapping and lookup
//
//----------------------------------------------

VNode* TableNodeModel::indexToNode( const QModelIndex & index) const
{
	if(index.isValid())
	{
		return static_cast<VNode*>(index.internalPointer());
	}
	return NULL;
}

QModelIndex TableNodeModel::nodeToIndex(const VNode* node, int column) const
{
	if(!node)
		return QModelIndex();

	int row=0;
	if((row=data_->posInFilter(node)) != -1)
	{
		return createIndex(row,column,const_cast<VNode*>(node));
	}
	return QModelIndex();
}


//Find the index for the node when we know what the server is!
QModelIndex TableNodeModel::nodeToIndex(VTableServer* server,const VNode* node, int column) const
{
	if(!node)
		return QModelIndex();

	int row=0;
	if((row=data_->posInFilter(server,node)) != -1)
	{
		return createIndex(row,column,const_cast<VNode*>(node));
	}
	return QModelIndex();
}

VInfo_ptr TableNodeModel::nodeInfo(const QModelIndex& index)
{
	VNode *n=indexToNode(index);
	if(n)
	{
		return VInfoNode::create(n);
	}

	VInfo_ptr info;
	return info;
}


//Server is about to be added
void TableNodeModel::slotServerAddBegin(int row)
{
    beginResetModel();
    //beginInsertRows(QModelIndex(),row,row);
}

//Addition of the new server has finished
void TableNodeModel::slotServerAddEnd()
{
    endResetModel();
    //endInsertRows();
}

//Server is about to be removed
void TableNodeModel::slotServerRemoveBegin(int row)
{
     beginResetModel();
     //beginRemoveRows(QModelIndex(),row,row);
}

//Removal of the server has finished
void TableNodeModel::slotServerRemoveEnd()
{
    endResetModel();
    //endRemoveRows();
}

//The node changed (it status etc)
void TableNodeModel::slotNodeChanged(VTableServer* server,const VNode* node)
{
	if(!node)
		return;

	QModelIndex index=nodeToIndex(server,node,0);

	if(!index.isValid())
		return;

	Q_EMIT dataChanged(index,index);
}

void TableNodeModel::slotBeginServerScan(VModelServer* server,int num)
{
    Q_ASSERT(active_ == true);
    Q_ASSERT(server);

	if(num >0)
	{
		int count=num;

		VNode* afterNode=NULL;
        int start=data_->pos(server->tableServer(),&afterNode);

		QModelIndex idx;

		if(afterNode)
		{
			idx=createIndex(start,0,afterNode);
		}

		beginInsertRows(idx,0,count-1);
	}
}

void TableNodeModel::slotEndServerScan(VModelServer* server,int num)
{
	assert(active_ == true);

	if(num >0)
		endInsertRows();


	Q_EMIT filterChanged();
}

void TableNodeModel::slotBeginServerClear(VModelServer* server,int num)
{
    Q_ASSERT(active_ == true);
    Q_ASSERT(server);

	if(num >0)
	{
		int start=-1;
		int count=-1;

		VNode* firstNode=NULL;
        data_->identifyInFilter(server->tableServer(),start,count,&firstNode);
		assert(firstNode);

		QModelIndex idx=createIndex(start,0,firstNode);
		beginRemoveRows(idx,0,count-1);
	}
}

void TableNodeModel::slotEndServerClear(VModelServer* server,int num)
{
	assert(active_ == true);

	if(num >0)
		endRemoveRows();
}
