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

#include "VFilter.hpp"
#include "ServerFilter.hpp"
#include "ServerHandler.hpp"
#include "VFilter.hpp"
#include "VNState.hpp"
#include "VSState.hpp"
#include "VAttribute.hpp"
#include "VNode.hpp"
#include "VIcon.hpp"
#include "VFileInfo.hpp"
#include "VModelData.hpp"

//=======================================
//
// TreeNodeModel
//
//=======================================

TreeNodeModel::TreeNodeModel(VModelData *data,AttributeFilter *atts,IconFilter* icons,QObject *parent) :
   AbstractNodeModel(data,icons,parent),
   atts_(atts)

{
	//Attribute filter changes
	connect(atts_,SIGNAL(changed()),
				this,SIGNAL(filterChanged()));

	//Icon filter changes
	connect(icons_,SIGNAL(changed()),
			this,SIGNAL(filterChanged()));
	//this,SLOT(slotIconFilterChanged()));


	//When the underlying data changes

	//Filter changed
	connect(data_,SIGNAL(filterChanged()),
				this,SIGNAL(filterChanged()));

	//Server added
	connect(data_,SIGNAL(serverAddBegin(int)),
				this,SLOT(slotServerAddBegin(int)));

	connect(data_,SIGNAL(serverAddEnd()),
								this,SLOT(slotServerAddEnd()));

	//Server removed
	connect(data_,SIGNAL(serverRemoveBegin(int)),
						this,SLOT(slotServerRemoveBegin(int)));

	connect(data_,SIGNAL(serverRemoveEnd()),
							this,SLOT(slotServerRemoveEnd()));

	//The whole server content changes
	connect(data_,SIGNAL(dataChanged(VModelServer*)),
			this,SLOT(slotDataChanged(VModelServer*)));

	//Node changes
	connect(data_,SIGNAL(nodeChanged(VModelServer*,const VNode*)),
				this,SLOT(slotNodeChanged(VModelServer*,const VNode*)));

	//Attributes change
	connect(data_,SIGNAL(attributesChanged(VModelServer*,const VNode*)),
			 	 this,SLOT(slotAttributesChanged(VModelServer*,const VNode*)));

	//Node or attributes number changed
	connect(data_,SIGNAL(addRemoveAttributes(VModelServer*,const VNode*,int,int)),
			this,SLOT(slotAddRemoveAttributes(VModelServer*,const VNode*,int,int)));

	connect(data_,SIGNAL(addRemoveNodes(VModelServer*,const VNode*,int,int)),
				this,SLOT(slotAddRemoveNodes(VModelServer*,const VNode*,int,int)));

	connect(data_,SIGNAL(addNode(VModelServer*,const VNode*,int)),
			this,SLOT(slotAddNode(VModelServer*,const VNode*,int)));

	connect(data_,SIGNAL(resetBranch(VModelServer*,const VNode*)),
			this,SLOT(slotResetBranch(VModelServer*,const VNode*)));

}

int TreeNodeModel::columnCount( const QModelIndex& /*parent */ ) const
{
   	 return 1;
}

int TreeNodeModel::rowCount( const QModelIndex& parent) const
{
	//qDebug() << "rowCount" << parent;

	//There is no data at all
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
		return data_->count();
	}
	//"parent" is a server
	else if(isServer(parent))
	{
		if(VModelServer *md=indexToServer(parent))
		{
			return md->topLevelNodeNum();
		}
	}
	//"parent" is a node
	else if(VNode* parentNode=indexToNode(parent))
	{
		return parentNode->attrNum()+parentNode->numOfChildren();
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
	   (role != Qt::DisplayRole && role != Qt::ToolTipRole && role != Qt::BackgroundRole &&
	    role != FilterRole && role != IconRole && role != ServerRole && role != NodeNumRole &&
	    role != ConnectedRole))
    {
		return QVariant();
	}

	//Identifies server
	if(role == ServerRole)
	{
		if(isServer(index))
			return 0;
		else
			return -1;
	}

	//Server
	if(isServer(index))
	{
		return serverData(index,role);
	}

	//We only continue for the relevant roles for nodes and attributes
	if(role == NodeNumRole || role == ConnectedRole)
	{
		return QVariant();
	}

	//Node
	if(isNode(index))
	{
		return nodeData(index,role);
	}

	//We only continue for the relevant roles for attributes
	if(role == IconRole)
	{
			return QVariant();
	}

	//Attribute
	else if(isAttribute(index))
		return attributesData(index,role);

	return QVariant();
}

QVariant TreeNodeModel::serverData(const QModelIndex& index,int role) const
{
	if(role == IconRole)
			return QVariant();

	if(role == FilterRole)
		return true;

	else if(index.column() == 0 && role == Qt::BackgroundRole)
	{
		if(ServerHandler *server=indexToRealServer(index))
				return server->vRoot()->stateColour();
		else
		    return Qt::gray;
	}

	else if(role == Qt::DisplayRole)
	{
		if(ServerHandler *server=indexToRealServer(index))
		{
				if(index.column() == 0)
					return QString::fromStdString(server->name());
				else if(index.column() == 1)
					return server->vRoot()->stateName();
		}
	}
	else if(role == NodeNumRole)
	{
		if(VModelServer *server=indexToServer(index))
		{
			return server->totalNodeNum();
		}
	}
	else if(role == ConnectedRole)
	{
		if(ServerHandler *server=indexToRealServer(index))
		{
			return server->connected();
		}
	}
	else if(role == Qt::ToolTipRole)
	{
		if(ServerHandler *server=indexToRealServer(index))
		{
			QString txt="<b>Server</b>: " + QString::fromStdString(server->name()) + "<br>";
			txt+="<b>Host</b>: " + QString::fromStdString(server->host());
			txt+=" <b>Port</b>: " + QString::fromStdString(server->port()) + "<br>";

			if(server->connected())
			{
				txt+="<b>Server status</b>: " + VSState::toName(server) + "<br>";
				txt+="<b>Status</b>: " + VNState::toName(server) + "<br>";
				txt+="<b>Total number of nodes</b>: " +  QString::number(server->vRoot()->totalNum());
			}
			else
			{
				txt+="<b>Server is disconnected!</b><br>";
				txt+="<b>Last connection attempt</b>: " + VFileInfo::formatDateAgo(server->lastConnectAttempt()) + "<br>";
				txt+="<b>Error message</b>:<br>" +  QString::fromStdString(server->connectError());
			}
			return txt;
		}
	}

	return QVariant();
}

QVariant TreeNodeModel::nodeData(const QModelIndex& index, int role) const
{
	VNode* vnode=indexToNode(index);
	if(!vnode || !vnode->node())
		return QVariant();

	Node *node=vnode->node();

	if(role == Qt::DisplayRole)
	{
		switch(index.column())
		{
		case 0:	 return QString::fromStdString(node->name());
		case 1: return VNState::toName(node);
		case 2: return QString::fromStdString(node->absNodePath()); //QString().sprintf("%08p", node); //QString::fromStdString(node->absNodePath());
		default: return QVariant();
		}
	}
	else if(role == FilterRole)
	{
		return QVariant(data_->isFiltered(vnode));
	}
	else if(index.column() == 0 && role == Qt::BackgroundRole)
	{
		return VNState::toColour(node);
	}
	else if(index.column() == 0 && role == IconRole)
	{
		if(icons_->isEmpty())
			return QVariant();
		else
			return VIcon::pixmapList(node,icons_);
	}
	else if(index.column() == 0 && role == Qt::ToolTipRole)
	{
		return VNState::toName(node);
	}


	return QVariant();
}

//=======================================================================
//
// Attributes data
//
//=======================================================================

QVariant TreeNodeModel::attributesData(const QModelIndex& index, int role) const
{
	if(role == IconRole)
			return QVariant();

	if(index.column()!=0)
		return QVariant();

	if(role != Qt::BackgroundRole && role != FilterRole && role != Qt::DisplayRole)
		return QVariant();

	if(role == Qt::BackgroundRole)
		return QColor(220,220,220);

	if(role == FilterRole)
	{
		if(atts_->isEmpty())
			return QVariant(false);
	}

	//Here we have to be sure this is an attribute!
	VNode *node=static_cast<VNode*>(index.internalPointer());
	if(!node)
		return QVariant();

	VAttribute* type;
	QStringList attrData=node->getAttributeData(index.row(),&type);

	//VParam::Type type;
	//VAttribute* type;
	//VAttribute::getData(node->node(),index.row(),&type,attrData);

	if(role == FilterRole)
	{
		if(atts_->isSet(type))
		{
			return QVariant(true);
		}
		return QVariant(false);
	}
	else if(role == Qt::DisplayRole)
	{
		return attrData;
	}


	/*

	//Here we have to be sure this is an attribute!
	Node *node=static_cast<Node*>(index.internalPointer());
	if(!node)
		return QVariant();

	QStringList attrData;
	//VParam::Type type;
	VAttribute* type;
	VAttribute::getData(node,index.row(),&type,attrData);

	if(role == FilterRole)
	{
		if(atts_->isSet(type))
		{
			return QVariant(true);
		}
		return QVariant(false);
	}
	else if(role == Qt::DisplayRole)
	{
		return attrData;
	}*/

	return QVariant();
}

QVariant TreeNodeModel::headerData( const int section, const Qt::Orientation orient , const int role ) const
{
	//No header!!!
	return QVariant();

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

	//When "parent" is the root this index refers to a server
	if(!parent.isValid())
	{
		//For the server the internal pointer is NULL
		if(row < data_->count())
		{
			void* p=NULL;
			//qDebug() << "SERVER" << parent;
			return createIndex(row,column,(void*)NULL);
		}
	}

	//Here we must be under one of the servers
	else
	{
		//If "parent" is a server this index refers to a topLevel node (suite).
		//We set the server as an internal pointer
		if(VModelServer* server=indexToServer(parent))
		{
			//qDebug() << "NODE1" << parent << server->realServer()->name().c_str();
			return createIndex(row,column,server);
		}

		//If "parent" is not a server it must be a node.
		else if(VNode* parentNode=indexToNode(parent))
		{
			//qDebug() << "NODE2" << parent << parentNode->name().c_str() << VAttribute::totalNum(parentNode);
			//qDebug() << "NODE2" << parent << parentNode->node()->name().c_str();
			return createIndex(row,column,parentNode);
		}

		//qDebug() << "BAD" << parent;
	}

	//qDebug() << "EMPTY" << parent;
	return QModelIndex();

}

QModelIndex TreeNodeModel::parent(const QModelIndex &child) const
{
	//If "child" is a server the parent is the root
	if(isServer(child))
		return QModelIndex();

	//Child is a topLevel node (suite). Its internal pointer points to a server.
	int row=-1;

	//If "child"'s internal pointer is a server (i.e. it is a topLevel node (suite))
	//the parent is this server.
	if((row=data_->indexOfServer(child.internalPointer())) != -1)
	{
		return createIndex(row,0,(void*)NULL);
	}

	//The "child" cannot be a topLevel node so it must be a node or an attribute.
	//Its internal pointer must point to the parent node.
	else if(VNode *parentNode=static_cast<VNode*>(child.internalPointer()))
	{
		//The parent is a topLevel node (suite): its internal pointer is the server
		if(parentNode->isTopLevel())
		{
			VModelServer *server=NULL;
			row=-1;
		    if(data_->identifyTopLevelNode(parentNode,&server,row))
			{
				//qDebug() << "PARENT 1" << child << server->realServer()->name().c_str();
		    	return createIndex(row,0,server);
			}
		}
		//The parent is a non topLevel node (non-suite): its internal pointer
		//is its parent (i.e.. the grandparent)
		else if(VNode *grandParentNode=parentNode->parent())
		{
			int num=grandParentNode->attrNum()+grandParentNode->indexOfChild(parentNode);

			//qDebug() << "PARENT 2" << child << grandParentNode->node()->name().c_str() << num;
			return createIndex(num,0,grandParentNode);
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
	//For the servers the internal pointer is NULL
	return (index.isValid() && index.internalPointer() == NULL);
}

bool TreeNodeModel::isNode(const QModelIndex & index) const
{
	return (indexToNode(index) != NULL);
}

bool TreeNodeModel::isAttribute(const QModelIndex & index) const
{
	return (index.isValid() && !isNode(index) && !isServer(index));
}

ServerHandler* TreeNodeModel::indexToRealServer(const QModelIndex & index) const
{
	//For servers the internal id is a nul pointer
	if(index.isValid())
	{
		if(index.internalPointer() == NULL)
			return data_->realServer(index.row());
	}

	return NULL;
}

VModelServer* TreeNodeModel::indexToServer(const QModelIndex & index) const
{
	//For servers the internal id is a nul pointer
	if(index.isValid())
	{
		if(index.internalPointer() == NULL)
			return data_->server(index.row());
	}

	return NULL;
}

QModelIndex TreeNodeModel::serverToIndex(ServerHandler* server) const
{
	//For servers the internal id is set to their position in servers_ + 1
	int i;
	if((i=data_->indexOfServer(server))!= -1)
			return createIndex(i,0,(void*)NULL);

	return QModelIndex();
}

VNode* TreeNodeModel::indexToNode( const QModelIndex & index) const
{
	//If it is not a sever ...
	if(index.isValid() && !isServer(index))
	{
		//If it is a top level node (suite) the internal pointer has to point
		//to a server pointer.
		if(VNode *n=data_->topLevelNode(index.internalPointer(),index.row()))
		{
			return n;
		}

		//Otherwise the internal pointer points to the parent node
		else if(VNode *parentNode=static_cast<VNode*>(index.internalPointer()))
		{
			int attNum=parentNode->attrNum();
			if(index.row() >= attNum)
			{
				return parentNode->childAt(index.row()-attNum);
			}
		}
	}

	return NULL;
}


//Find the index for the node!
QModelIndex TreeNodeModel::nodeToIndex(const VNode* node, int column) const
{
	if(!node)
		return QModelIndex();

	//If the node is toplevel node (suite).
	if(node->isTopLevel())
	{
		VModelServer* server=NULL;
		int row=-1;
		data_->identifyTopLevelNode(node,&server,row);
		{
			return createIndex(row,column,server);
		}
	}
	//Other nodes
	else if(VNode *parentNode=node->parent())
	{
		int row=parentNode->indexOfChild(node);
		if(row != -1)
		{
			row+=parentNode->attrNum();
			return createIndex(row,column,parentNode);
		}
	}

	return QModelIndex();
}

//Find the index for the node when we know what the server is!
QModelIndex TreeNodeModel::nodeToIndex(VModelServer* server,const VNode* node, int column) const
{
	if(!node)
		return QModelIndex();

	//If the node is toplevel node (suite).
	if(node->isTopLevel())
	{
		int row=server->indexOfTopLevelNode(node);
		if(row != -1)
		{
			return createIndex(row,column,server);
		}
	}
	//Other nodes
	else if(VNode *parentNode=node->parent())
	{
		int row=parentNode->indexOfChild(node);
		if(row != -1)
		{
			row+=parentNode->attrNum();
			return createIndex(row,column,parentNode);
		}
	}

	return QModelIndex();

}



/*
//Find the index for the node!
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
			int attNum=VAttribute::totalNum(parentNode);
			row+=attNum;
			return createIndex(row,column,parentNode);
		}
	}

	return QModelIndex();
}
*/
//Find the index for the node when we know what the server is!
/*QModelIndex TreeNodeModel::nodeToIndex(ServerHandler* server,VNode* node, int column) const
{
	if(!node)
		return QModelIndex();

	//If the node is a suite
	if(node->isSuite())
	{
		if(server)
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
			int attNum=VAttribute::totalNum(parentNode);
			row+=attNum;
			return createIndex(row,column,parentNode);
		}
	}

	return QModelIndex();
}*/

//------------------------------------------------------------------
// Create info object to index. It is used to identify nodes in
// the tree all over in the programme outside the view.
//------------------------------------------------------------------

VInfo_ptr TreeNodeModel::nodeInfo(const QModelIndex& index)
{
	//For invalid index no info is created.
	if(!index.isValid())
	{
		VInfo_ptr res;
		return res;
	}

	//Check if the node is a server
	if(ServerHandler *s=indexToRealServer(index))
	{
		VInfo_ptr res(VInfo::make(s));
		return res;
	}

	//If it is a top level node (suite) the internal pointer has to point
	//to a server pointer.
	if(VNode *n=data_->topLevelNode(index.internalPointer(),index.row()))
	{
		VInfo_ptr res(VInfo::make(n));
		return res;
	}

	//Otherwise the internal pointer points to the parent node
	else if(VNode *parentNode=static_cast<VNode*>(index.internalPointer()))
	{
		int attNum=parentNode->attrNum();
		if(index.row() >= attNum)
		{
			VNode *n=parentNode->childAt(index.row()-attNum);
			VInfo_ptr res(VInfo::make(n));
			return res;
		}
		//It is an attribute
		else
		{

		}
	}

    VInfo_ptr res;
	return res;
}

//----------------------------------------
// Filter
//----------------------------------------

//This slot is called when the icon filter changes
void TreeNodeModel::slotIconFilterChanged()
{
	//This should trigger the re-rendering of the whole view according to
	//some blogs. However, this does not work!!!!!
	//Q_EMIT dataChanged(QModelIndex(), QModelIndex());

	//This is very expensive!! There should be a better way!!!
	Q_EMIT filterChanged();
}

//Server is about to be added
void TreeNodeModel::slotServerAddBegin(int row)
{
	beginInsertRows(QModelIndex(),row,row);
}

//Addition of the new server has finished
void TreeNodeModel::slotServerAddEnd()
{
	endInsertRows();
}

//Server is about to be removed
void TreeNodeModel::slotServerRemoveBegin(int row)
{
	beginRemoveRows(QModelIndex(),row,row);
}

//Removal of the server has finished
void TreeNodeModel::slotServerRemoveEnd()
{
	endRemoveRows();
}

void TreeNodeModel::slotDataChanged(VModelServer* server)
{

}

//The node changed (it status etc)
void TreeNodeModel::slotNodeChanged(VModelServer* server,const VNode* node)
{
	if(!node)
		return;

	QModelIndex index=nodeToIndex(server,node,0);

	if(!index.isValid())
		return;

	Q_EMIT dataChanged(index,index);
}

//One of the attributes of the node changed. The total number of the
//attributes is the same.
void TreeNodeModel::slotAttributesChanged(VModelServer* server,const VNode* node)
{
	if(!node)
		return;

	//Find the index of the node.
	QModelIndex parent=nodeToIndex(server,node,0);
	if(!parent.isValid())
		return;

	//Find out the indexes for all the attributes. For an attribute
	//the internal pointer of the index points to the parent VNode.
	QModelIndex idx1=index(0,0,parent);
	QModelIndex idx2=index(node->attrNum()-1,0,parent);

	Q_EMIT dataChanged(idx1,idx2);
}

//Attributes were added or removed
void TreeNodeModel::slotAddRemoveAttributes(VModelServer* server,const VNode* node,int currentNum,int cachedNum)
{
	int diff=currentNum-cachedNum;
	if(diff==0)
		return;

	//Find the index of the node. This call does not use the
	//number of attributes of the node so it is safe.
	QModelIndex parent=nodeToIndex(server,node,0);
	if(!parent.isValid())
		return;

	//At this point the model state is based on cachedNum!!!!

	//Insertion
	if(diff > 0)
	{
		//We add extra rows to the end of the attributes
		beginInsertRows(parent,cachedNum,cachedNum+diff-1);
		endInsertRows();
	}
	//Deletion
	else if(diff <0)
	{
		//We remove rows from the end
		beginRemoveRows(parent,cachedNum+diff,cachedNum-1);
		endRemoveRows();
	}

	//At this point the model state is based on currentNum!!!!
}

//Nodes were added or removed
void TreeNodeModel::slotAddRemoveNodes(VModelServer* server,const VNode* node,int currentNum,int cachedNum)
{
	int diff=currentNum-cachedNum;
	if(diff==0)
		return;

	//Find the index of the node. This call does not use the
	//number of attributes of the node so it is safe.
	QModelIndex parent=nodeToIndex(server,node,0);
	if(!parent.isValid())
		return;

	int attrNum=node->attrNum();

	//At this point the model state is based on cachedNum!!!!
	//Insertion
	if(diff > 0)
	{
		//We add extra rows to the end of the attributes
		beginInsertRows(parent,attrNum+cachedNum,attrNum+cachedNum+diff-1);
		endInsertRows();
	}
	//Deletion
	else if(diff <0)
	{
		//We remove rows from the end
		beginRemoveRows(parent,attrNum+cachedNum+diff,attrNum+cachedNum-1);
		endRemoveRows();
	}

	//At this point the model state is based on currentNum!!!!
}



//A new node was added. posInNodes tells us the position within the nodes before
//the new node has to be inserted.
void TreeNodeModel::slotAddNode(VModelServer* server,const VNode* node,int posInNodes)
{
	if(!node)
		return;

	//Find the index of the node.
	QModelIndex parent=nodeToIndex(server,node,0);
	if(!parent.isValid())
		return;

	int attrNum=node->attrNum();
	beginInsertRows(parent,attrNum+posInNodes,attrNum+posInNodes+1);
	endInsertRows();
}

//The whole branch belonging to the node has to be reset!!!
void TreeNodeModel::slotResetBranch(VModelServer* server,const VNode* node)
{
	if(!node)
		return;

	//brute force approach

	QModelIndex idx=nodeToIndex(server,node,0);
	QModelIndex parentIdx=parent(idx);

	//Remove the node
	beginRemoveRows(parentIdx,idx.row(),idx.row());
	endRemoveRows();

	//Add it back again
	beginInsertRows(parentIdx,idx.row(),idx.row());
	endInsertRows();

}
