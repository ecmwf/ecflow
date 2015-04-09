//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "AbstractNodeModel.hpp"

#include <QDebug>

#include "ChangeMgrSingleton.hpp"

#include "ServerFilter.hpp"
#include "ServerHandler.hpp"
#include "ServerItem.hpp"
#include "VFilter.hpp"
#include "VModelData.hpp"
#include "VNState.hpp"


AbstractNodeModel::AbstractNodeModel(VModelData *data,IconFilter* icons,QObject *parent) :
   QAbstractItemModel(parent),
   data_(data),
   icons_(icons),
   active_(false)
{
	//At this point the model is not active and it cannot see its data!
}

AbstractNodeModel::~AbstractNodeModel()
{
	clean();
}

void AbstractNodeModel::active(bool active)
{
	if(active_ != active)
	{
		active_=active;

		beginResetModel();

		//When the model becomes active we reload everything
		if(active_)
		{
			data_->runFilter(false);
			//init();

			//Initialises the filter
			//resetStateFilter(false);
		}

		//When the model becomes inactive we clean it and
		//release all the resources
		else
		{
			data_->clear(); //clean();
		}

		endResetModel();

		//After finishing reset the view will automatically be notified about
		//the changes. Also the filter model will be notified!
	}
}

//Called when the list of servers to be displayed has changed.
//void AbstractNodeModel::notifyConfigChanged(ServerFilter*)
//{
//	if(active_)
//		reload();
//}

void AbstractNodeModel::init()
{
	/*ServerFilter *filter=config_->serverFilter();
	for(unsigned int i=0; i < filter->items().size(); i++)
	{
		if(ServerHandler *server=filter->items().at(i)->serverHandler())
		{
		    //The model has to observe the nodes o the server.
		    server->addNodeObserver(this);

		    //The model stores the servers it has to deal with in a local object.
		    servers_->add(server,makeFilter());
		}
	}*/
}

void AbstractNodeModel::clean()
{
	/*for(int i=0; i < servers_->count(); i++)
	{
		if(ServerHandler *s=servers_->server(i))
		{
			s->removeNodeObserver(this);
		}
	}

	servers_->clear();*/
}

//TODO!!!!!

//Should be reviewed what it is actually doing!!!!!!!!!!!!!!!!!
void AbstractNodeModel::reload()
{
	if(active_)
	{
		beginResetModel();
		//data_->reload();
		//clean();
		//init();
		//resetStateFilter(false); //do not emit change signal
		endResetModel();
	}
}


bool AbstractNodeModel::hasData() const
{
	return (active_&& data_->count() > 0);
}

void AbstractNodeModel::dataIsAboutToChange()
{
	beginResetModel();
}

/*void AbstractNodeModel::addServer(ServerHandler *server)
{
	//servers_ << server;
	//rootNodes_[servers_.back()] = NULL;
}*/


/*Node * AbstractNodeModel::rootNode(ServerHandler* server) const
{
	QMap<ServerHandler*,Node*>::const_iterator it=rootNodes_.find(server);
	if(it != rootNodes_.end())
		return it.value();
	return NULL;
}*/


/*void AbstractNodeModel::setRootNode(Node *node)
{
	if(ServerHandler *server=ServerHandler::find(node))
	{
		beginResetModel();

		rootNodes_[server]=node;

		//Reset the model (views will be notified)
		endResetModel();

		qDebug() << "setRootNode finished";
	}
}*/

//----------------------------------------------
//
// Server to index mapping and lookup
//
//----------------------------------------------

QModelIndex AbstractNodeModel::infoToIndex(VInfo_ptr info,int column) const
{
	if(info)
	{
		if(info->isServer())
		{
			if(ServerHandler *s=info->server())
			{
				return serverToIndex(s);
			}
		}
		else if(VNode* n=info->node())
		{
			return nodeToIndex(n);
		}
	}

	return QModelIndex();
}

/*VInfo_ptr AbstractNodeModel::nodeInfo(const QModelIndex& index)
{
	if(!index.isValid())
	{
		VInfo_ptr res;
		return res;
	}

	ServerHandler *server=indexToServer(index);
	if(server)
	{
		VInfo_ptr res(VInfo::make(server));
		return res;
	}
	else
	{
		Node* node=indexToNode(index);
		VInfo_ptr res(VInfo::make(node));
		return res;
	}
}*/


/*void AbstractNodeModel::notifyNodeChanged(const Node* node, const std::vector<ecf::Aspect::Type>& types)
{
	if(node==NULL)
		return;

	qDebug() << "observer is called" << QString::fromStdString(node->name());
	//for(unsigned int i=0; i < types.size(); i++)
	//	qDebug() << "  type:" << types.at(i);

	Node* nc=const_cast<Node*>(node);

	QModelIndex index1=nodeToIndex(nc,0);
	QModelIndex index2=nodeToIndex(nc,2);

	if(!index1.isValid() || !index2.isValid())
		return;

	Node *nd1=indexToNode(index1);
	Node *nd2=indexToNode(index2);

	if(!nd1 || !nd2)
		return;

	//qDebug() << "indexes" << index1 << index2;
	//qDebug() << "index pointers " << index1.internalPointer() << index2.internalPointer();
	qDebug() << "    --->" << QString::fromStdString(nd1->name()) << QString::fromStdString(nd2->name());

	Q_EMIT dataChanged(index1,index2);
}*/





