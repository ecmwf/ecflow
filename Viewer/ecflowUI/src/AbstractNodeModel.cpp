//============================================================================
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "AbstractNodeModel.hpp"

#include <QDebug>


#include "ServerFilter.hpp"
#include "ServerHandler.hpp"
#include "ServerItem.hpp"
#include "VFilter.hpp"
#include "VModelData.hpp"
#include "VNState.hpp"


AbstractNodeModel::AbstractNodeModel(QObject *parent) :
   QAbstractItemModel(parent),
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

        data()->setActive(active_);

#if 0
		//When the model becomes active we reload everything
		if(active_)
		{
            data()->runFilter(false);
			//init();

			//Initialises the filter
			//resetStateFilter(false);
		}

		//When the model becomes inactive we clean it and
		//release all the resources
		else
		{
			data()->clear(); //clean();
		}
#endif
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
	return (active_ && data()->count() > 0);
}

void AbstractNodeModel::dataIsAboutToChange()
{
	beginResetModel();
}

void AbstractNodeModel::slotFilterDeleteBegin()
{
	beginResetModel();
}


void  AbstractNodeModel::slotFilterDeleteEnd()
{
	endResetModel();
}


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
        else if(info->isNode())
        {
            VNode* n=info->node();
            return nodeToIndex(n);  
        }
        else if(info->isAttribute())
        {
            VAttribute* a=info->attribute();
            return attributeToIndex(a);     
        }
	}

	return QModelIndex();
}
