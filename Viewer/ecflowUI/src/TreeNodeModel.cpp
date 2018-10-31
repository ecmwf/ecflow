//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "TreeNodeModel.hpp"

#include <QMetaMethod>

#include "ConnectState.hpp"
#include "VFilter.hpp"
#include "ServerFilter.hpp"
#include "ServerHandler.hpp"
#include "UiLog.hpp"
#include "VFilter.hpp"
#include "VNState.hpp"
#include "VSState.hpp"
#include "VAttribute.hpp"
#include "VAttributeType.hpp"
#include "VNode.hpp"
#include "VIcon.hpp"
#include "VFileInfo.hpp"
#include "VModelData.hpp"
#include "VTree.hpp"

//#define _UI_TREENODEMODEL_DEBUG

//=======================================
//
// TreeNodeModel
//
//=======================================

TreeNodeModel::TreeNodeModel(ServerFilter* serverFilter,NodeFilterDef* filterDef,
		                     AttributeFilter *atts,IconFilter* icons,QObject *parent) :
   AbstractNodeModel(parent),
   data_(nullptr),
   atts_(atts),
   icons_(icons),
   serverToolTip_(true),
   nodeToolTip_(true),
   attributeToolTip_(true)
{
	//Create the data handler for the tree model.
    data_=new VTreeModelData(filterDef,atts_,this);

    //Reset the data handler
	data_->reset(serverFilter);

    //Icon filter changes
    connect(icons_,SIGNAL(changed()),
            this,SIGNAL(rerender()));

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
        if(VTreeServer *server=indexToServer(parent))
		{
            if(server->inScan())
                return 0;
            else
                return server->tree()->attrNum(atts_)+server->tree()->numOfChildren();
		}
	}
	//"parent" is a node
    else if(VTreeNode* parentNode=indexToNode(parent))
	{
        return parentNode->attrNum(atts_)+parentNode->numOfChildren();
	}

	return 0;
}

Qt::ItemFlags TreeNodeModel::flags ( const QModelIndex & index) const
{
	Qt::ItemFlags defaultFlags;

	defaultFlags=Qt::ItemIsEnabled | Qt::ItemIsSelectable;

	return defaultFlags;
}

//This function can be called billions of times, so it has to be extremely fast.
QVariant TreeNodeModel::data(const QModelIndex& index, int role ) const
{
    //Data lookup can be costly so we only enter the function for the valid cases
    if(index.isValid() && (role > Qt::UserRole || role == Qt::DisplayRole ||
       role == Qt::ToolTipRole || role == Qt::BackgroundRole ||
       role == Qt::ForegroundRole))
    {
        //At this point we know that the index is valid so we do not need to
        //check it anymore.

        //This role is called millions of times when expanding or
        //relayouting huge trees. So has to be HIGHLY optimised!!!!!
        if(role == AttributeLineRole)
        {
            if(VTreeNode* node=indexToAttrParentNode(index))
            {
                VNode *vnode=node->vnode();
                Q_ASSERT(vnode);
                if(VAttribute* a=vnode->attribute(index.row(),atts_))
                {
                    return a->lineNum();
                }
            }
            return 0;
        }

        //Identifies server
        else if(role == ServerRole)
        {
            if(isServerForValid(index))
                return 0;
            else
                return -1;
        }

        else if(role == AttributeRole)
        {
            return isAttribute(index);
        }

        //Server
        else if(isServerForValid(index))
        {
            return serverData(index,role);
        }

        //If we are here we can be sure that the index in not a server!

        //We only continue for the relevant roles for nodes and attributes
        if(role == InfoRole || role == LoadRole)
        {
            return QVariant();
        }

        bool itIsANode=false;
        if(VTreeNode* node=indexToAttrParentOrNode(index,itIsANode))
        {
            //Attribute
            if(itIsANode ==false)
            {
                return attributesData(index,role,node);
            }

            //Node
            if(itIsANode)
            {
                return nodeData(index,role,node);
            }
        }
    }

	return QVariant();
}

QVariant TreeNodeModel::serverData(const QModelIndex& index,int role) const
{
	if(role == FilterRole)
		return true;

    if(role == Qt::ToolTipRole && !serverToolTip_)
        return QVariant();

    ServerHandler *server=indexToServerHandler(index);
	if(!server)
		return QVariant();

	if(index.column() == 0)
	{
        if(role == ServerPointerRole)
            return qVariantFromValue((void *) server);

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

QVariant TreeNodeModel::nodeData(const QModelIndex& index, int role,VTreeNode* tnode) const
{
    if(role == Qt::ToolTipRole && !nodeToolTip_)
        return QVariant();

    if(!tnode)
        return QVariant();

    VNode* vnode=tnode->vnode();
	if(!vnode || !vnode->node())
		return QVariant();

    if(role == NodePointerRole)
        return qVariantFromValue((void *) vnode);

    else if(role == ConnectionRole)
    {
        return (vnode->server()->connectState()->state() == ConnectState::Lost)?0:1;
    }

    else if(role == Qt::DisplayRole)
        return vnode->name();

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
         else if(vnode->isAlias()) return 3;
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
            if(data_->isFilterComplete())
                return vnode->server()->vRoot()->totalNumOfTopLevel(vnode);
            else
                return  QString::number(tnode->root()->totalNumOfTopLevel(tnode)) + "/" +
                        QString::number(vnode->server()->vRoot()->totalNumOfTopLevel(vnode));
        }
        return QVariant();
    }
    else if(role == AbortedReasonRole && vnode->isAborted())
    {
        return QString::fromStdString(vnode->abortedReason());
    }
    else if(role == FailedSubmissionRole)
    {
        return vnode->isFlagSet(ecf::Flag::JOBCMD_FAILED);
    }

	return QVariant();
}

//=======================================================================
//
// Attributes data
//
//=======================================================================

QVariant TreeNodeModel::attributesData(const QModelIndex& index, int role,VTreeNode* tnode) const
{
    if(role == Qt::ToolTipRole && !attributeToolTip_)
        return QVariant();

	if(role == Qt::BackgroundRole)
		return QColor(220,220,220);

    VNode *vnode=tnode->vnode();
    Q_ASSERT(vnode);

    if(role == ConnectionRole)
	{
        return (vnode->server()->connectState()->state() == ConnectState::Lost)?0:1;
	}
	else if(role == Qt::DisplayRole)
    {
        if(VAttribute* a=vnode->attribute(index.row(),atts_))
            return a->data();
        else
            return QStringList();
	}
    else if(role ==  Qt::ToolTipRole)
    {
        if(VAttribute* a=vnode->attribute(index.row(),atts_))
            return a->toolTip();
        else
            return QString();
    }

	return QVariant();
}

QVariant TreeNodeModel::headerData( const int section, const Qt::Orientation orient , const int role ) const
{
	//No header!!!
	return QVariant();
}

QModelIndex TreeNodeModel::index( int row, int column, const QModelIndex & parent ) const
{
	if(!hasData() || row < 0 || column < 0)
	{
		return {};
	}

	//When "parent" is the root this index refers to a server
	if(!parent.isValid())
	{
		//For the server the internal pointer is NULL
		if(row < data_->count())
		{			
			return createIndex(row,column,(void*)nullptr);
		}
	}

	//Here we must be under one of the servers
	else
	{
		//If "parent" is a server this index refers to a topLevel node (suite).
		//We set the server as an internal pointer
        if(VTreeServer* server=indexToServer(parent))
		{			
			return createIndex(row,column,server);
		}

        //If "parent" is not a server it must be a tree node. The internal pointer is the parent tree node.
        else if(VTreeNode* parentNode=indexToNode(parent))
		{			
			return createIndex(row,column,parentNode);
		}
	}
	return QModelIndex();
}

QModelIndex TreeNodeModel::parent(const QModelIndex &child) const
{
	//If "child" is a server the parent is the root
	if(isServer(child))
		return {};

	int row=-1;

	//If the "child"'s internal pointer is a server it can be a server attribute or a topLevel node (suite)
	//and the parent is this server.
	if((row=data_->indexOfServer(child.internalPointer())) != -1)
	{
		return createIndex(row,0,(void*)nullptr);
	}

	//The "child" cannot be a server attribute or a topLevel node so it must be a node or an attribute.
	//Its internal pointer must point to the parent node.
    else if(auto *parentNode=static_cast<VTreeNode*>(child.internalPointer()))
	{
		//The parent is a topLevel node (suite): its internal pointer is the server
		if(parentNode->isTopLevel())
		{
            VTreeNode* root=parentNode->parent();
            Q_ASSERT(root);
            row=root->indexOfChild(parentNode);
            Q_ASSERT(row >=0);
            VTreeServer *ts=root->server();

            int serverAttrNum=root->attrNum(atts_);
            return createIndex(serverAttrNum+row,0,ts);
		}
		//The parent is a non topLevel node (non-suite): its internal pointer
		//is its parent (i.e.. the grandparent)
        else if(VTreeNode *grandParentNode=parentNode->parent())
		{
            int num=grandParentNode->attrNum(atts_)+grandParentNode->indexOfChild(parentNode);		
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
    return (index.isValid() && index.internalPointer() == nullptr);
}

//This has to be extrememly fast!
//When we know that the index is valid.
bool TreeNodeModel::isServerForValid(const QModelIndex & index) const
{
    //For the servers the internal pointer is NULL
    return index.internalPointer() == nullptr;
}

bool TreeNodeModel::isNode(const QModelIndex & index) const
{
    return (indexToNode(index) != nullptr);
}

// A node is a flat node when:
// -there are no child nodes (only attributes)
// or
// -any child node does not have children or attributes
bool TreeNodeModel::isFlatNode(const QModelIndex& index) const
{
    if(VTreeNode *node=indexToNode(index))
    {
        if(node->numOfChildren() == 0)
            return true;
        else
        {
            for(int i=0; i < node->numOfChildren(); i++)
            {
                if(node->childAt(i)->numOfChildren() == 0)
                {
                   if(node->childAt(i)->attrNum(atts_) > 0)
                       return false;
                }
                else
                {
                   return false;
                }
            }
            return true;
        }
    }
    return false;
}

bool TreeNodeModel::isAttribute(const QModelIndex & index) const
{
	return (index.isValid() && !isServer(index) && !isNode(index));
}

ServerHandler* TreeNodeModel::indexToServerHandler(const QModelIndex & index) const
{
	//For servers the internal id is a null pointer
	if(index.isValid())
	{
		if(index.internalPointer() == nullptr)
            return data_->serverHandler(index.row());
	}

	return nullptr;
}

VTreeServer* TreeNodeModel::indexToServer(const QModelIndex & index) const
{
	//For servers the internal id is a null pointer
	if(index.isValid())
	{
		if(index.internalPointer() == nullptr)
            return data_->server(index.row())->treeServer();
	}

	return nullptr;
}

VTreeServer* TreeNodeModel::nameToServer(const std::string& name) const
{
     VModelServer* ms=data_->server(name);
     return (ms)?ms->treeServer():nullptr;
}

QModelIndex TreeNodeModel::serverToIndex(ServerHandler* server) const
{
	//For servers the internal id is set to their position in servers_ + 1
	int i;
	if((i=data_->indexOfServer(server))!= -1)
			return createIndex(i,0,(void*)nullptr);

	return {};
}

QModelIndex TreeNodeModel::serverToIndex(VModelServer* server) const
{
	//For servers the internal id is set to their position in servers_ + 1
	int i;
	if((i=data_->indexOfServer(server))!= -1)
			return createIndex(i,0,(void*)nullptr);

	return {};
}

//----------------------------------------------
//
// Node to index mapping and lookup
//
//----------------------------------------------

//We can only call it when the index is valid!
VTreeNode* TreeNodeModel::indexToAttrParentNode(const QModelIndex & index) const
{
    void *ip;
    if((ip=index.internalPointer()) == nullptr)
        return nullptr;

    //If it is not a sever ...

    //If the internal pointer is a server it is either a server attribute or a
    //top level node (suite)

    if(VModelServer *mserver=data_->server(ip))
    {
        VTreeServer* server=mserver->treeServer();
        //It is an attribute
        if(index.row() < server->tree()->attrNum(atts_))
        {
            return server->tree();
        }
    }

    //Otherwise the internal pointer points to the parent node.
    else if(auto *parentNode=static_cast<VTreeNode*>(ip))
    {
        //It is an attribute
        if(index.row() < parentNode->attrNum(atts_))
            return parentNode;
    }

    return nullptr;
}

//We can only call it when the index is valid!
VTreeNode* TreeNodeModel::indexToAttrParentOrNode(const QModelIndex & index,bool &itIsANode) const
{
    void *ip;
    if((ip=index.internalPointer()) == nullptr)
        return nullptr;

    //If it is not a sever ...

    itIsANode=false;
    //If the internal pointer is a server it is either a server attribute or a
    //top level node (suite)
    if(VModelServer *mserver=data_->server(ip))
    {
        VTreeServer* server=mserver->treeServer();
        Q_ASSERT(server);

        //It is an attribute
        int serverAttNum=server->tree()->attrNum(atts_);

        if(index.row() < serverAttNum)
        {
            return server->tree();
        }
        //It is a top level node
        else
        {
            itIsANode=true;
            return server->tree()->childAt(index.row()-serverAttNum);
        }
    }

    //Otherwise the internal pointer points to the parent node.
    else if(auto *parentNode=static_cast<VTreeNode*>(ip))
    {
        int attNum=parentNode->attrNum(atts_);

        //It is an attribute
        if(index.row() < attNum)
        {
            return parentNode;
        }
        else
        {
            itIsANode=true;
            return parentNode->childAt(index.row()-attNum);
        }
    }

    return nullptr;
}

VTreeNode* TreeNodeModel::indexToServerOrNode( const QModelIndex & index) const
{
    VTreeNode* node=indexToNode(index);
    if(!node)
    {
        if(VTreeServer* ts=indexToServer(index))
            node=ts->tree();
    }

    return node;
}

VTreeNode* TreeNodeModel::indexToNode( const QModelIndex & index) const
{
	//If it is not a sever ...
	if(index.isValid() && !isServer(index))
	{
		//If the internal pointer is a server it is either a server attribute or a
		//top level node (suite)

        if(VModelServer *mserver=data_->server(index.internalPointer()))
		{
            VTreeServer* server=mserver->treeServer();
            Q_ASSERT(server);

            int serverAttNum=server->tree()->attrNum(atts_);

			//It is an attribute
			if(index.row() < serverAttNum)
			{
				return nullptr;
			}
			//It is a top level node
			else
			{
                return server->tree()->childAt(index.row()-serverAttNum);
			}
		}

		//Otherwise the internal pointer points to the parent node.
        else if(auto *parentNode=static_cast<VTreeNode*>(index.internalPointer()))
		{
            int attNum=parentNode->attrNum(atts_);
			if(index.row() >= attNum)
			{
                return parentNode->childAt(index.row()-attNum);
			}
		}
	}

	return nullptr;
}

//Find the index for the node! The VNode can be a server as well!!!
QModelIndex TreeNodeModel::nodeToIndex(const VNode* node, int column) const
{
    if(!node)
		return {};

	//This is a server!!!
	if(node->isServer())
	{
		return serverToIndex(node->server());
	}
	//If the node is toplevel node (suite).
	else if(node->isTopLevel())
	{
        if(VModelServer *mserver=data_->server(node->server()))
        {
            VTreeServer* server=mserver->treeServer();
            Q_ASSERT(server);

            //the node is displayed in the tree
            if(VTreeNode* tn=server->tree()->find(node))
            {
                int row=tn->indexInParent();
                Q_ASSERT(tn->parent() == server->tree());
                Q_ASSERT(row >=0);
                row+=server->tree()->attrNum(atts_);
                return createIndex(row,column,server);
            }
        }
	}

    //Other nodes
    else if(VNode *parentNode=node->parent())
	{
        if(VModelServer *mserver=data_->server(node->server()))
        {
            VTreeServer* server=mserver->treeServer();
            Q_ASSERT(server);

            if(VTreeNode* tn=server->tree()->find(node))
            {
                VTreeNode* tnParent=tn->parent();
                Q_ASSERT(tnParent);
                Q_ASSERT(tnParent->vnode() == parentNode);
                int row=tnParent->indexOfChild(tn);
                row+=tnParent->attrNum(atts_);
                return createIndex(row,column,tnParent);
            }
        }
	}

	return QModelIndex();
}

//Find the index for the node
QModelIndex TreeNodeModel::nodeToIndex(const VTreeNode* node, int column) const
{
    if(!node)
        return {};

    //It is a server
    if(node->parent() == nullptr)
    {
        VTreeServer *server=node->server();
        Q_ASSERT(server);
        return serverToIndex(server);
    }
    else if(node->isTopLevel())
    {
        VTree *vt=node->root();
        Q_ASSERT(vt);
        Q_ASSERT(vt == node->parent());
        int row=vt->indexOfChild(node);
        if(row != -1)
        {
           row+=vt->attrNum(atts_);
           return createIndex(row,column,vt->server());
        }

    }
    if(VTreeNode *parentNode=node->parent())
    {
        int row=parentNode->indexOfChild(node);
        if(row != -1)
        {
            row+=parentNode->attrNum(atts_);
            return createIndex(row,column,parentNode);
        }
    }

    return QModelIndex();

}

//Find the index for the node when we know what the server is!
QModelIndex TreeNodeModel::nodeToIndex(VTreeServer* server,const VTreeNode* node, int column) const
{
    if(!node)
		return {};

	//If the node is toplevel node (suite).
	if(node->isTopLevel())
	{
        Q_ASSERT(node->parent() == server->tree());
        int row=server->tree()->indexOfChild(node);
        Q_ASSERT(row >=0);

        row+=server->tree()->attrNum(atts_);
        return createIndex(row,column,server);
	}
	//Other nodes
    else if(VTreeNode *parentNode=node->parent())
	{
		int row=parentNode->indexOfChild(node);
		if(row != -1)
		{
            row+=parentNode->attrNum(atts_);
			return createIndex(row,column,parentNode);
		}
	}

	return QModelIndex();

}

//Find the index for the node! The VNode can be a server as well!!!
QModelIndex TreeNodeModel::attributeToIndex(const VAttribute* a, int column) const
{
    if(!a)
        return {};

    VNode* node=a->parent();
    if(!node)
        return QModelIndex();

    VModelServer *mserver=data_->server(node->server());
    VTreeServer* server=mserver->treeServer();
    Q_ASSERT(server);

    int row=node->indexOfAttribute(a,atts_);
    if(row != -1)
    {
        //This is a server!!!
        if(node->isServer())
        {
            return createIndex(row,column,server);
        }
        else
        {
            if(VTreeNode* tn=server->tree()->find(node))
            {
                return createIndex(row,column,tn);
            }
        }
    }

    return QModelIndex();
}

#if 0
QModelIndex TreeNodeModel::forceShowNode(const VNode* node) const
{
    //There is nothing to do when there is no status filter set
    if(data_->isFilterNull())
        return QModelIndex();

    Q_ASSERT(node);
    Q_ASSERT(!node->isServer());
    Q_ASSERT(node->server());

    if(VModelServer *mserver=data_->server(node->server()))
    {
        VTreeServer* server=mserver->treeServer();
        Q_ASSERT(server);
        server->setForceShowNode(node);
        return nodeToIndex(node);
    }

    return QModelIndex();
}

QModelIndex TreeNodeModel::forceShowAttribute(const VAttribute* a) const
{
    VNode* node=a->parent();
    Q_ASSERT(node);
    //Q_ASSERT(!node->isServer());
    Q_ASSERT(node->server());

    if(VModelServer *mserver=data_->server(node->server()))
    {
        VTreeServer* server=mserver->treeServer();
        Q_ASSERT(server);        
        server->setForceShowAttribute(a);
        return attributeToIndex(a);
    }

    return QModelIndex();
}

#endif

//Sets the object strored in info as the current forceShow object in the
//data. A forceShow object must always be part of the tree whatever state or
//atribute filter is set. There can be only one forceShow object at a time in the 
//whole model/tree.
void TreeNodeModel::setForceShow(VInfo_ptr info)
{
    UI_FUNCTION_LOG

    if(info)
        UiLog().dbg() << " info=" << info->path();

    //Clear the forceShow object in all the server data objects. Only one of 
    //these is actually need to clear it.
    for(int i=0; i < data_->count(); i++)
    {
        VTreeServer *ts=data_->server(i)->treeServer();
        Q_ASSERT(ts);
        ts->clearForceShow(info->item());
    }
    
    //Sets the forceShow objects in all the server data objects. Only one of these will
    //actually set the forceShow.
    for(int i=0; i < data_->count(); i++)
    {
        VTreeServer *ts=data_->server(i)->treeServer();
        Q_ASSERT(ts);
        ts->setForceShow(info->item());
    }    
}

//The selection changed in the view
void TreeNodeModel::selectionChanged(QModelIndexList lst)
{
    UI_FUNCTION_LOG

    //if(data_->isFilterNull())
    //    return;

    if(lst.count() > 0)
    {
        QModelIndex idx=lst.back();

        VInfo_ptr info=nodeInfo(idx);

        setForceShow(info);
    }
        

    #if 0
    Q_FOREACH(QModelIndex idx,lst)
    {
        VInfo_ptr info=nodeInfo(idx);

        for(int i=0; i < data_->count(); i++)
        {
           VTreeServer *ts=data_->server(i)->treeServer();
           Q_ASSERT(ts);
           ts->clearForceShow(info->item());
        }

        for(int i=0; i < data_->count(); i++)
        {
           VTreeServer *ts=data_->server(i)->treeServer();
           Q_ASSERT(ts);
           ts->setForceShow(info->item());
        }
    }
    #endif
}


//------------------------------------------------------------------
// Create info object to index. It is used to identify nodes in
// the tree all over in the programme outside the view.
//------------------------------------------------------------------

//WARNING: if we are in the middle of an attribute filter change it will not give
//correct results, because atts_ contains the new filter state, but the whole VTree still
//base on the previous atts_ state!! However we can assume that the index vas visible in
//the tree so attrNum() is cached on the tree nodes so we get correct results for nodes.
//Attributes however cannot be identified correctly.

VInfo_ptr TreeNodeModel::nodeInfo(const QModelIndex& index)
{
	//For invalid index no info is created.
	if(!index.isValid())
	{
		VInfo_ptr res;
		return res;
	}

	//Check if the node is a server
    if(ServerHandler *s=indexToServerHandler(index))
	{
		return VInfoServer::create(s);
	}

	//If the internal pointer is a server it is either a server attribute or a
	//top level node (suite)
    if(VModelServer *mserver=data_->server(index.internalPointer()))
	{
        VTreeServer *server=mserver->treeServer();
        Q_ASSERT(server);

        //If the attrNum is cached it is correct!
        int serverAttNum=server->tree()->attrNum(atts_);

		//It is a top level node
		if(index.row() >= serverAttNum)
		{
            VNode *n=server->tree()->childAt(index.row()-serverAttNum)->vnode();
			return VInfoNode::create(n);
		}
        else
        {
            //TODO: serverattribute
        }
	}

	//Otherwise the internal pointer points to the parent node
    else if(auto *parentNode=static_cast<VTreeNode*>(index.internalPointer()))
	{       
        //If the attrNum is cached it is correct!
        int attNum=parentNode->attrNum(atts_);

        //It is a node
        if(index.row() >= attNum)
        {
            VNode *n=parentNode->childAt(index.row()-attNum)->vnode();
            return VInfoNode::create(n);
        }
        //It is an attribute
        else
        {
            //This wil not work properly if we are in the middle of an attribute
            //filter change! atts_ is the new filter state, but index.row() is based on
            //the previous filter state!!
            if(VAttribute* a=parentNode->vnode()->attribute(index.row(),atts_))
            {
                VInfo_ptr p=VInfoAttribute::create(a);
                return p;

            }
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
void TreeNodeModel::slotServerRemoveBegin(VModelServer* server,int /*nodeNum*/)
{
#ifdef _UI_TREENODEMODEL_DEBUG
    UiLog().dbg() << "TreeNodeModel::slotServerRemoveBegin -->";
#endif

    int row=data_->indexOfServer(server);
    Q_ASSERT(row >= 0);

#ifdef _UI_TREENODEMODEL_DEBUG
    UiLog().dbg() << "  row: " << row;
#endif
	beginRemoveRows(QModelIndex(),row,row);
}

//Removal of the server has finished
void TreeNodeModel::slotServerRemoveEnd(int /*nodeNum*/)
{
#ifdef _UI_TREENODEMODEL_DEBUG
    UiLog().dbg() << "TreeNodeModel::slotServerRemoveEnd -->";
#endif

    endRemoveRows();
}

void TreeNodeModel::slotDataChanged(VModelServer* server)
{
	QModelIndex idx=serverToIndex(server);
	Q_EMIT dataChanged(idx,idx);
}

//The node changed (it status etc)
void TreeNodeModel::slotNodeChanged(VTreeServer* server,const VTreeNode* node)
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
void TreeNodeModel::slotAttributesChanged(VTreeServer* server,const VTreeNode* node)
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
    QModelIndex idx2=index(node->attrNum(atts_)-1,0,parent);

	Q_EMIT dataChanged(idx1,idx2);
}

//Attributes were added or removed
void TreeNodeModel::slotBeginAddRemoveAttributes(VTreeServer* server,const VTreeNode* node,int currentNum,int cachedNum)
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
void TreeNodeModel::slotEndAddRemoveAttributes(VTreeServer* server,const VTreeNode* node,int currentNum,int cachedNum)
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
    //slot below will update all the attributes of the node in the tree).
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

    VTreeServer *ts=server->treeServer();
    Q_ASSERT(ts);
    Q_EMIT scanEnded(ts->tree());

    if(ts->isFirstScan())
        Q_EMIT firstScanEnded(ts);
}

//The server clear has started. It will remove all the nodes except the root node.
//So we need to remove all the rows belonging to the rootnode.
void TreeNodeModel::slotBeginServerClear(VModelServer* server,int)
{
	assert(active_ == true);

	QModelIndex idx=serverToIndex(server);

	if(idx.isValid())
	{
        VTreeServer *ts=server->treeServer();
        Q_ASSERT(ts);
        Q_EMIT clearBegun(ts->tree());

        //We removes the attributes as well!!!
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

void TreeNodeModel::slotBeginFilterUpdateRemove(VTreeServer* server,const VTreeNode* topChange,int totalRows)
{
    Q_ASSERT(topChange);
    Q_ASSERT(!topChange->isRoot());

    QModelIndex idx=nodeToIndex(server,topChange);
    int attrNum=topChange->attrNum(atts_);
    int chNum=topChange->numOfChildren();
    int totalNum=attrNum+chNum;

    Q_ASSERT(totalNum == totalRows);
    Q_ASSERT(attrNum >=0);
    Q_ASSERT(chNum >=0);

    if(totalRows >0)
    {
        Q_EMIT filterUpdateRemoveBegun(topChange);
        beginRemoveRows(idx,attrNum,attrNum+chNum-1);
    }
}

void TreeNodeModel::slotEndFilterUpdateRemove(VTreeServer* /*server*/,const VTreeNode* topChange,int totalRows)
{
    Q_ASSERT(!topChange->isRoot());

    if(totalRows >0)
    {
        endRemoveRows();
    }
}

void TreeNodeModel::slotBeginFilterUpdateAdd(VTreeServer* server,const VTreeNode* topChange,int chNum)
{
    Q_ASSERT(topChange);
    Q_ASSERT(!topChange->isRoot());

    QModelIndex idx=nodeToIndex(server,topChange);
    int attrNum=topChange->attrNum(atts_);
    //int totalNum=attrNum+chNum;

    Q_ASSERT(attrNum >=0);
    Q_ASSERT(chNum >=0);

    if(chNum >0)
    {
        beginInsertRows(idx,attrNum,attrNum+chNum-1);
    }
}

void TreeNodeModel::slotEndFilterUpdateAdd(VTreeServer* /*server*/,const VTreeNode* topChange,int chNum)
{
    Q_ASSERT(topChange);
    Q_ASSERT(!topChange->isRoot());

    if(chNum >0)
    {
        endInsertRows();
    }

    Q_EMIT filterUpdateAddEnded(topChange);
}

void TreeNodeModel::slotBeginFilterUpdateRemoveTop(VTreeServer* server,int row)
{
    Q_ASSERT(server);
    QModelIndex idx=serverToIndex(server);
    int attrNum=server->tree()->attrNum(atts_);
    int chNum=server->tree()->numOfChildren();

    Q_ASSERT(chNum > row);
    Q_ASSERT(attrNum >=0);
    Q_ASSERT(chNum >=0);

    beginRemoveRows(idx,attrNum+row,attrNum+row);
}

void TreeNodeModel::slotEndFilterUpdateRemoveTop(VTreeServer* server,int)
{
    Q_ASSERT(server);
    endRemoveRows();
}

void TreeNodeModel::slotBeginFilterUpdateInsertTop(VTreeServer* server,int row)
{
    Q_ASSERT(server);
    QModelIndex idx=serverToIndex(server);
    int attrNum=server->tree()->attrNum(atts_);
    int chNum=server->tree()->numOfChildren();

    Q_ASSERT(chNum >= row);
    Q_ASSERT(attrNum >=0);
    Q_ASSERT(chNum >=0);

    beginInsertRows(idx,attrNum+row,attrNum+row);
}

void TreeNodeModel::slotEndFilterUpdateInsertTop(VTreeServer* server,int row)
{
    Q_ASSERT(server);
    endInsertRows();

    int attrNum=server->tree()->attrNum(atts_);
    int chNum=server->tree()->numOfChildren();

    Q_ASSERT(chNum >= row);
    Q_ASSERT(attrNum >=0);
    Q_ASSERT(chNum >=0);

    if(row >=0)
    {
        const VTreeNode* topChange=server->tree()->childAt(row);
        //when a suite becomes visible we must notify the view about it so that
        //the expand state could be correctly set!!!
        Q_EMIT filterUpdateAddEnded(topChange);
    }
}


int TreeNodeModel::iconNum(VNode* n) const
{
    if(icons_->isEmpty())
        return 0;
    else
        return VIcon::pixmapNum(n,icons_);
}

