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
#include "UserMessage.hpp"
#include "VFilter.hpp"
#include "VIcon.hpp"
#include "VModelData.hpp"
#include "VNode.hpp"
#include "VNState.hpp"

#define _UI_TABLENODEMODEL_DEBUG

//static int hitCount=0;

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

    Q_ASSERT(columns_);

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
#ifdef _UI_TABLENODEMODEL_DEBUG
    //qDebug() << "rowCount" << parent;
#endif

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
#ifdef _UI_TABLENODEMODEL_DEBUG
        //qDebug() << "table count" << cnt;
#endif
		return cnt;
	}

	return 0;
}


QVariant TableNodeModel::data( const QModelIndex& index, int role ) const
{
	//Data lookup can be costly so we immediately return a default value for all
	//the cases where the default should be used.
	if( !index.isValid() ||
       (role != Qt::DisplayRole && role != Qt::ToolTipRole &&
        role != Qt::BackgroundRole && role != IconRole))
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
        {   return QString::fromStdString(vnode->absNodePath());
        }
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
    if(!hasData() || row < 0 || column < 0 || parent.isValid())
	{
		return QModelIndex();
	}

    if(VNode *node=data_->nodeAt(row))
	{
		return createIndex(row,column,node);
	}

	return QModelIndex();
}

QModelIndex TableNodeModel::parent(const QModelIndex& /*child*/) const
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
    if((row=data_->position(node)) != -1)
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
    if((row=data_->position(server,node)) != -1)
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
void TableNodeModel::slotServerAddBegin(int /*row*/)
{
}

//Addition of the new server has finished
void TableNodeModel::slotServerAddEnd()
{
}

//Server is about to be removed
void TableNodeModel::slotServerRemoveBegin(VModelServer* server,int num)
{
    Q_ASSERT(active_ == true);
    Q_ASSERT(server);

    if(num >0)
    {
        int start=-1;
        int count=-1;
        data_->position(server->tableServer(),start,count);

        Q_ASSERT(start >=0);
        Q_ASSERT(count == num);

        beginRemoveRows(QModelIndex(),start,start+count-1);
    }
}

//Removal of the server has finished
void TableNodeModel::slotServerRemoveEnd(int num)
{
    assert(active_ == true);

    if(num >0)
        endRemoveRows();
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

#ifdef _UI_TABLENODEMODEL_DEBUG
     UserMessage::debug("TableNodeModel::slotBeginServerScan --> " + server->realServer()->name() + " " + QString::number(num).toStdString());
#endif

	if(num >0)
	{
		int count=num;
        int start=data_->position(server->tableServer());
        beginInsertRows(QModelIndex(),start,start+count-1);
	}
}

void TableNodeModel::slotEndServerScan(VModelServer* server,int num)
{
	assert(active_ == true);

#ifdef _UI_TABLENODEMODEL_DEBUG
     UserMessage::debug("TableNodeModel::slotEndServerScan --> " + server->realServer()->name() + " " + QString::number(num).toStdString());
     QTime t;
     t.start();
#endif

	if(num >0)
		endInsertRows();

#ifdef _UI_TABLENODEMODEL_DEBUG
     UserMessage::debug("  elapsed: " + QString::number(t.elapsed()).toStdString() + " ms");
     UserMessage::debug("<-- TableNodeModel::slotEndServerScan");

     //qDebug() << "hit" << hitCount;
#endif
}

void TableNodeModel::slotBeginServerClear(VModelServer* server,int num)
{
    Q_ASSERT(active_ == true);
    Q_ASSERT(server);

	if(num >0)
	{
		int start=-1;
		int count=-1;
        data_->position(server->tableServer(),start,count);

        Q_ASSERT(start >=0);
        Q_ASSERT(count == num);

        beginRemoveRows(QModelIndex(),start,start+count-1);
	}
}

void TableNodeModel::slotEndServerClear(VModelServer* server,int num)
{
	assert(active_ == true);

	if(num >0)
		endRemoveRows();
}
