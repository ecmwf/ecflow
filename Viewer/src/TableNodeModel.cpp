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

#include "ChangeMgrSingleton.hpp"

#include "ServerHandler.hpp"
#include "VFilter.hpp"
#include "VModelData.hpp"
#include "VNode.hpp"
#include "VNState.hpp"

//=======================================================
//
// TableNodeModel
//
//=======================================================

TableNodeModel::TableNodeModel(VModelData *data,IconFilter* icons,QObject *parent) :
	AbstractNodeModel(data,icons,parent)
{

	//Filter changed
	connect(data_,SIGNAL(filterChanged()),
			this,SIGNAL(filterChanged()));

	//We need connect the rest of the VModelData signals to local slots

	//Loop over the methods of the VModelServer
	for(int i = 0; i < data_->staticMetaObject.methodCount(); i++)
	{
		//Get the method signature
		const char* sg=data_->staticMetaObject.method(i).signature();

		//Check if it is a signal
		if(data_->staticMetaObject.indexOfSignal(sg) != -1)
		{
			QString localSlot(sg);
			if(!localSlot.isEmpty())
			{
				QChar first=localSlot.at(0);

				localSlot="slot" + localSlot.replace(0,1,first.toUpper());

				//Check if it is a slot in the current class as well
				int idx=staticMetaObject.indexOfSlot(localSlot.toStdString().c_str());
				if(idx != -1)
				{
					//We simply relay this signal
					connect(data_,data_->staticMetaObject.method(i),
							this,staticMetaObject.method(idx));
				}
			}
		}
	}
}

int TableNodeModel::columnCount( const QModelIndex& /*parent */ ) const
{
   	 return 3;
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
		//We the total number of nodes displayed
		int cnt=0;
		for(int i=0; i < data_->count(); i++)
		{
			cnt+=data_->numOfFiltered(i);
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
	   (role != Qt::DisplayRole && role != Qt::ToolTipRole && role != Qt::BackgroundRole && role != FilterRole))
    {
		return QVariant();
	}

	//We only display nodes!!
	return nodeData(index,role);
}

QVariant TableNodeModel::nodeData(const QModelIndex& index, int role) const
{
	if(role == Qt::BackgroundRole && index.column() != 1)
		return QVariant();

	qDebug() << "data" <<  index;


	VNode* vnode=indexToNode(index);
	if(!vnode || !vnode->node())
		return QVariant();
	if(role == Qt::DisplayRole)
	{
		switch(index.column())
		{
		case 0: return QString::fromStdString(vnode->absNodePath());
		case 1: return vnode->stateName();
		case 2:
		{
				ServerHandler* s=vnode->server();
				return (s)?QString::fromStdString(s->name()):QString();
		}
		default: return QVariant();
		}
	}
	else if(role == Qt::BackgroundRole)
	{
		return vnode->stateColour();
	}
	else if(role == FilterRole)
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

/*bool TableNodeModel::isServer(const QModelIndex & index) const
{
	//For servers the internal id is set to their position in servers_ + 1
	if(index.isValid())
	{
		int id=index.internalId()-1;
		return (id >=0 && id < data_->count());
	}
	return false;
}*/


/*ServerHandler* TableNodeModel::indexToRealServer(const QModelIndex & index) const
{
	//For servers the internal id is set to their position in servers_ + 1
	if(index.isValid())
	{
		int id=index.internalId()-1;
		return data_->realServer(id);
	}
	return NULL;
}

VModelServer* TableNodeModel::indexToServer(const QModelIndex & index) const
{
	//For servers the internal id is set to their position in servers_ + 1
	if(index.isValid())
	{
		int id=index.internalId()-1;
		return data_->server(id);
	}
	return NULL;
}
*/

/*QModelIndex TableNodeModel::serverToIndex(ServerHandler* server) const
{
	//For servers the internal id is set to their position in servers_ + 1
	int i;
	if((i=data_->indexOfServer(server))!= -1)
			return createIndex(i,0,i+1);

	return QModelIndex();
}

QModelIndex TableNodeModel::serverToIndex(VModelServer* server) const
{
	//For servers the internal id is set to their position in servers_ + 1
	int i;
	if((i=data_->indexOfServer(server))!= -1)
			return createIndex(i,0,i+1);

	return QModelIndex();
}*/


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
		return createIndex(row,column,(VNode*)node);
	}
	return QModelIndex();
}


//Find the index for the node when we know what the server is!
QModelIndex TableNodeModel::nodeToIndex(VModelServer* server,const VNode* node, int column) const
{
	if(!node)
		return QModelIndex();

	int row=0;
	if((row=data_->posInFilter(server,node)) != -1)
	{
		return createIndex(row,column,(VNode*)node);
	}
	return QModelIndex();
}



VInfo_ptr TableNodeModel::nodeInfo(const QModelIndex&)
{
	VInfo_ptr info;
	return info;
}

//The node changed (it status etc)
void TableNodeModel::slotNodeChanged(VModelServer* server,const VNode* node)
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
	assert(active_ == true);

	if(num >0)
	{
		int start=-1;
		int count=-1;

		VNode* firstNode=NULL;
		data_->identifyInFilter(server,start,count,&firstNode);
		assert(firstNode);

		QModelIndex idx=createIndex(start,0,firstNode);
		beginInsertRows(idx,0,count-1);
	}
}

void TableNodeModel::slotEndServerScan(VModelServer* server,int num)
{
	assert(active_ == true);

	if(num >0)
		endInsertRows();
}

void TableNodeModel::slotBeginServerClear(VModelServer* server,int num)
{
	assert(active_ == true);

	if(num >0)
	{
		int start=-1;
		int count=-1;

		VNode* firstNode=NULL;
		data_->identifyInFilter(server,start,count,&firstNode);
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
