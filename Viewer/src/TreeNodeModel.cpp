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
#include <QMetaMethod>

#include "ConnectState.hpp"
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

TreeNodeModel::TreeNodeModel(ServerFilter* serverFilter,NodeFilterDef* filterDef,
		                     AttributeFilter *atts,IconFilter* icons,QObject *parent) :
   AbstractNodeModel(parent),
   data_(0),
   atts_(atts),
   icons_(icons)
{
	//Create the data handler for the tree model.
	data_=new VTreeModelData(filterDef,this);

	data_->reset(serverFilter);

	//---------------------------------------
	// Handle change in the filters' state
	//---------------------------------------

	//TODO
	//We could use this
	//
	//  Q_EMIT dataChanged(QModelIndex(), QModelIndex());
	//
	//because this should trigger the re-rendering of the whole view according to
	//some blogs. However, this does not work!!!!! So we emit the filterChanged() signal that
	//will finally call invalidate() in the filter model.
	//
	//However it is very expensive!! There should be a better way!!!

	//Attribute filter changes
	connect(atts_,SIGNAL(changed()),
				this,SIGNAL(filterChanged()));

	//Icon filter changes
	connect(icons_,SIGNAL(changed()),
			this,SIGNAL(filterChanged()));

	//State filter changes (handled in data_)!!!
}


VModelData* TreeNodeModel::data() const
{
	return data_;
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
			return md->attrNum()+md->topLevelNodeNum();
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
	    role != Qt::ForegroundRole &&
	    role != FilterRole && role != IconRole && role != ServerRole && role != NodeNumRole &&
	    role != InfoRole && role != LoadRole && role != ConnectionRole && role != AttributeRole && role != AttributeLineRole &&
        role != AbortedReasonRole && role != NodeTypeRole && role != NodeTypeForegroundRole))
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

	if(role == AttributeRole)
	{
		return isAttribute(index);
	}

	//Server
	if(isServer(index))
	{
		if(role == AttributeLineRole)
			return 0;

		return serverData(index,role);
	}

	//We only continue for the relevant roles for nodes and attributes
	if(role == InfoRole || role == LoadRole)
	{
		return QVariant();
	}

	//Node
	if(isNode(index))
	{
		if(role == AttributeLineRole)
			return 0;

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
	if(role == FilterRole)
		return true;

	ServerHandler *server=indexToRealServer(index);
	if(!server)
		return QVariant();

	if(index.column() == 0)
	{
		//The colour of the server node
		if(role == ConnectionRole)
			return (server->connectState()->state() == ConnectState::Lost)?0:1;

		//The colour of the server node
		else if(role == Qt::BackgroundRole)
			return server->vRoot()->stateColour();

		//The font colour of the server node
		else if(role == Qt::ForegroundRole)
			return server->vRoot()->stateFontColour();

		//The text
		else if(role == Qt::DisplayRole)
			return QString::fromStdString(server->name());

		//The number of nodes the server has
		else if(role == NodeNumRole)
		{
			ConnectState* st=server->connectState();
			if(server->activity() != ServerHandler::LoadActivity)
			{
				return server->vRoot()->totalNum();
			}
			return QVariant();
		}

		//Extra information about the server activity
		else if(role == InfoRole)
		{
			switch(server->activity())
			{
			case ServerHandler::LoadActivity:
				return "Loading ...";
			default:
				return "";
			}
		}

		else if(role == LoadRole)
			return (server->activity() == ServerHandler::LoadActivity);

		//icon decoration
		/*else if(role == IconRole)
		{
			//TODO: add a proper iconprovider
			ConnectState* st=server->connectState();
			if(server->activity() != ServerHandler::LoadActivity &&
			   st->state() != ConnectState::Normal)
			{
				return "d";
			}
			return QVariant();
		}*/

		else if(role == IconRole)
		{
			if(icons_->isEmpty())
				return QVariant();
			else
				return VIcon::pixmapList(server->vRoot(),icons_);
		}

		//Tooltip
		else if(role == Qt::ToolTipRole)
		{
			QString txt=server->vRoot()->toolTip();
			txt+=VIcon::toolTip(server->vRoot(),icons_);
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

	if(index.column() == 0)
	{
		//The colour of the server node
		if(role == ConnectionRole)
		{
			return (vnode->server()->connectState()->state() == ConnectState::Lost)?0:1;
		}

		else if(role == Qt::DisplayRole)
			return vnode->name();

		else if(role == FilterRole)
			return QVariant(data_->isFiltered(vnode));

		else if(role == Qt::BackgroundRole)
		{
			if(vnode->isSuspended())
			{
				QVariantList lst;
				lst << vnode->stateColour() << vnode->realStateColour();
				return lst;
			}
			else
				return vnode->stateColour() ;
		}

		else if(role == Qt::ForegroundRole)
			return vnode->stateFontColour();

        else if(role  == NodeTypeRole)
        {
            if(vnode->isTask()) return 2;
            else if(vnode->isSuite()) return 0;
            else if(vnode->isFamily()) return 1;
            return 0;
        }
        else if(role  == NodeTypeForegroundRole)
        {
            return vnode->typeFontColour();
        }
		else if(role == IconRole)
		{
			if(icons_->isEmpty())
				return QVariant();
			else
				return VIcon::pixmapList(vnode,icons_);
		}
		else if(role == Qt::ToolTipRole)
		{
			QString txt=vnode->toolTip();
			txt+=VIcon::toolTip(vnode,icons_);
			return txt;
		}

		//The number of nodes a suite has
		else if(role == NodeNumRole)
		{
			if(vnode->isTopLevel())
			{
				return vnode->server()->vRoot()->totalNumOfTopLevel(vnode);
			}
			return QVariant();
		}

		//The number of nodes a suite has
		else if(role == AbortedReasonRole && vnode->isAborted())
		{
			return QString::fromStdString(vnode->abortedReason());
		}
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

	if(role != Qt::BackgroundRole && role != FilterRole && role != Qt::DisplayRole && role != ConnectionRole && role != AttributeLineRole )
		return QVariant();

	if(role == Qt::BackgroundRole)
		return QColor(220,220,220);

	if(role == FilterRole)
	{
		if(atts_->isEmpty())
			return QVariant(false);
	}

	VNode *node=NULL;

	//Here we have to be sure this is an attribute!

	//Server attribute
	VModelServer *server=data_->server(index.internalPointer());
	if(server)
	{
		node=server->realServer()->vRoot();
	}
	//Node attribute
	else
	{
		node=static_cast<VNode*>(index.internalPointer());
	}

	if(!node)
		return QVariant();


	if(role == FilterRole)
	{
		VAttribute* type=node->getAttributeType(index.row());
		if(atts_->isSet(type))
		{
			return QVariant(true);
		}
		return QVariant(false);
	}

	else if(role == ConnectionRole)
	{
		return (node->server()->connectState()->state() == ConnectState::Lost)?0:1;
	}

	else if(role == Qt::DisplayRole)
	{
		VAttribute* type=0;
		return node->getAttributeData(index.row(),type);
	}
	else if(role ==  AttributeLineRole)
	{
		return node->getAttributeLineNum(index.row());
	}

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

	int row=-1;

	//If the "child"'s internal pointer is a server it can be a server attribute or a topLevel node (suite)
	//and the parent is this server.
	if((row=data_->indexOfServer(child.internalPointer())) != -1)
	{
		return createIndex(row,0,(void*)NULL);
	}

	//The "child" cannot be a server attribute or a topLevel node so it must be a node or an attribute.
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
				int serverAttrNum=server->attrNum();

		    	//qDebug() << "PARENT 1" << child << server->realServer()->name().c_str();
		    	return createIndex(serverAttrNum+row,0,server);
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
	return (index.isValid() && !isServer(index) && !isNode(index));
}

ServerHandler* TreeNodeModel::indexToRealServer(const QModelIndex & index) const
{
	//For servers the internal id is a null pointer
	if(index.isValid())
	{
		if(index.internalPointer() == NULL)
			return data_->realServer(index.row());
	}

	return NULL;
}

VModelServer* TreeNodeModel::indexToServer(const QModelIndex & index) const
{
	//For servers the internal id is a null pointer
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

QModelIndex TreeNodeModel::serverToIndex(VModelServer* server) const
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
		//If the internal pointer is a server it is either a server attribute or a
		//top level node (suite)
		if(VModelServer *server=data_->server(index.internalPointer()))
		{
			int serverAttNum=server->attrNum();

			//It is an attribute
			if(index.row() < serverAttNum)
			{
				return NULL;
			}
			//It is a top level node
			else
			{
				return server->topLevelNode(index.row()-serverAttNum);
			}
		}

		//Otherwise the internal pointer points to the parent node.
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


//Find the index for the node! The VNode can be a server as well!!!
QModelIndex TreeNodeModel::nodeToIndex(const VNode* node, int column) const
{
	if(!node)
		return QModelIndex();

	//This is a server!!!
	if(node->isServer())
	{
		return serverToIndex(node->server());
	}
	//If the node is toplevel node (suite).
	else if(node->isTopLevel())
	{
		VModelServer* server=NULL;
		int row=-1;
		data_->identifyTopLevelNode(node,&server,row);
		{
			row+=server->attrNum();
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
			row+=server->attrNum();
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
		return VInfoServer::create(s);
	}


	//If the internal pointer is a server it is either a server attribute or a
	//top level node (suite)
	if(VModelServer *server=data_->server(index.internalPointer()))
	{
		int serverAttNum=server->attrNum();

		//It is a top level node
		if(index.row() >= serverAttNum)
		{
			VNode *n=server->topLevelNode(index.row()-serverAttNum);
			return VInfoNode::create(n);
		}
	}

	//Otherwise the internal pointer points to the parent node
	else if(VNode *parentNode=static_cast<VNode*>(index.internalPointer()))
	{
		int attNum=parentNode->attrNum();
		if(index.row() >= attNum)
		{
			VNode *n=parentNode->childAt(index.row()-attNum);
			return VInfoNode::create(n);
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
// Slots
//----------------------------------------

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
	QModelIndex idx=serverToIndex(server);
	Q_EMIT dataChanged(idx,idx);
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
void TreeNodeModel::slotBeginAddRemoveAttributes(VModelServer* server,const VNode* node,int currentNum,int cachedNum)
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
	//So VNode::attr() must return cachedNum and we need to pretend we have
	//cachedNum number of attributes

	//Insertion
	if(diff > 0)
	{
		//We add extra rows to the end of the attributes
		beginInsertRows(parent,cachedNum,cachedNum+diff-1);
	}
	//Deletion
	else if(diff <0)
	{
		//We remove rows from the end
		beginRemoveRows(parent,cachedNum+diff,cachedNum-1);
	}

	//At this point the model state is based on currentNum!!!!
}

//Attributes were added or removed
void TreeNodeModel::slotEndAddRemoveAttributes(VModelServer* server,const VNode* node,int currentNum,int cachedNum)
{
	int diff=currentNum-cachedNum;
	if(diff==0)
		return;

	//Find the index of the node. This call does not use the
	//number of attributes of the node so it is safe.
	QModelIndex parent=nodeToIndex(server,node,0);
	if(!parent.isValid())
		return;

	//At this point the model state is based on currentNum!!!!

	//Insertion
	if(diff > 0)
	{
		endInsertRows();
	}
	//Deletion
	else if(diff <0)
	{
		endRemoveRows();
	}

	//We need to update all the attributes to reflect the change!
	//(Since we do not have information about what attribute was actually added
	//we always add or remove attributes at the end of the attribute list!!! Then the
	//call below will update all the attributes of the node in the tree).
	slotAttributesChanged(server,node);
}

//The server scan has started (to populate the tree). At this point the tree is empty only containing the
//root node. Num tells us the number of children nodes (aka suites) the root node will contain.
void TreeNodeModel::slotBeginServerScan(VModelServer* server,int num)
{
	assert(active_ == true);

	QModelIndex idx=serverToIndex(server);

	//At this point the server node does not have any rows in the model!!!
	if(idx.isValid() && num >0)
	{
		beginInsertRows(idx,0,num-1);
	}
}

//The server scan has finished. The tree is fully populated.
void TreeNodeModel::slotEndServerScan(VModelServer* server,int num)
{
	assert(active_ == true);

	QModelIndex idx=serverToIndex(server);
	if(idx.isValid() && num >0)
	{
		endInsertRows();
	}
	Q_EMIT scanEnded(server->realServer()->vRoot());
}

//The server clear has started. It well remove all the nodes except the root node.
//So we need to remove all the rows belonging to the rootnode.
void TreeNodeModel::slotBeginServerClear(VModelServer* server,int)
{
	assert(active_ == true);

	QModelIndex idx=serverToIndex(server);

	if(idx.isValid())
	{
		Q_EMIT clearBegun(server->realServer()->vRoot());

		int num=rowCount(idx);
		beginRemoveRows(idx,0,num-1);
	}
}
//The server clear has finished. The tree is empty only containing the rootnode
void TreeNodeModel::slotEndServerClear(VModelServer* server,int)
{
	assert(active_ == true);
	endRemoveRows();
}
