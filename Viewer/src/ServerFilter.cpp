//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "ServerFilter.hpp"

#include "ServerHandler.hpp"
#include "ServerItem.hpp"
#include "ServerList.hpp"
#include "VSettings.hpp"

/*
ServerFilterItem::ServerFilterItem(const std::string& name) //,const std::string& host,const std::string& port)
{
	server_=ServerItem::find(name); //,host,port);
	server_->addObserver(this);

	ServerHandler* h=ServerHandler::find(server_->host(),server_->port());
	if(!h)
		ServerHandler::addServer(server_->host(),server_->port());
}

ServerFilterItem::~ServerFilterItem()
{
}

bool ServerFilterItem::hasSuiteFilter()
{
	return (suiteFilter_.size()>0);
}

void ServerFilterItem::addToSuiteFilter(const std::string& suite)
{
	suiteFilter_.insert(suite);
}

void ServerFilterItem::removeFromSuiteFilter(const std::string& suite)
{
	std::set<std::string>::iterator it=suiteFilter_.find(suite);
	if(it != suiteFilter_.end())
		suiteFilter_.erase(it);
}

ServerHandler* ServerFilterItem::serverHandler() const
{
	if(server_)
	{
		return ServerHandler::find(server_->host(),server_->port());
	}

	return 0;
}
*/

//==============================================
//
// ServerFilter
//
//==============================================

ServerFilter::ServerFilter()
{
}

ServerFilter::~ServerFilter()
{
	for(std::vector<ServerItem*>::const_iterator it=items_.begin(); it != items_.end(); ++it)
	{
		(*it)->removeObserver(this);
	}
}


void ServerFilter::addServer(ServerItem *item,bool broadcast)
{
	if(item && ServerList::instance()->find(item->name()) == item)
	{
			//ServerFilterItem* s=new ServerFilterItem(item->name(),item->host(),item->port());
			//ServerItem* s=new ServerFilterItem(item->name());

			items_.push_back(item);

			item->addObserver(this);

			if(broadcast)
				broadcastAdd(item);
	}
}

void ServerFilter::removeServer(ServerItem *server)
{
	if(!server) return;

	std::vector<ServerItem*>::iterator it=std::find(items_.begin(),items_.end(),server);
	if(it != items_.end())
	{
		//Remove the item from the filter. This should come
	    //first because the observers update themselves according to the
		//contents of items_!!!!
		items_.erase(it);

		//Notifies the view about the changes
		broadcastRemove(server);

		//Remove the filter from the observers
		server->removeObserver(this);
	}
}

void ServerFilter::notifyServerItemChanged(ServerItem *server)
{
	if(isFiltered(server))
		broadcastChange(server);
}

//Do not remove the observer in this method!!
void ServerFilter::notifyServerItemDeletion(ServerItem *server)
{
	if(!server) return;

	std::vector<ServerItem*>::iterator it=std::find(items_.begin(),items_.end(),server);
	if(it != items_.end())
	{
		items_.erase(it);

		//Notifies the view about the changes
		broadcastRemove(server);
	}
}

bool ServerFilter::isFiltered(ServerItem* item) const
{
	for(std::vector<ServerItem*>::const_iterator it=items_.begin(); it != items_.end(); ++it)
	{
		if((*it) == item)
			return true;
	}
	return false;
}

void ServerFilter::writeSettings(VSettings* vs) const
{
	std::vector<std::string> array;
	for(std::vector<ServerItem*>::const_iterator it=items_.begin(); it != items_.end(); ++it)
	{
		array.push_back((*it)->name());
	}

	vs->put("server",array);
}

void ServerFilter::readSettings(VSettings* vs)
{
	items_.clear();

	std::vector<std::string> array;
	vs->get("server",array);

	for(std::vector<std::string>::const_iterator it = array.begin(); it != array.end(); ++it)
	{
			std::string name=*it;
			if(ServerItem* s=ServerList::instance()->find(name))
			{
				addServer(s,false);
			}
	}
}

//===========================================================
// Observers
//===========================================================

void ServerFilter::broadcastAdd(ServerItem *server)
{
	for(std::vector<ServerFilterObserver*>::const_iterator it=observers_.begin(); it != observers_.end(); ++it)
	{
			(*it)->notifyServerFilterAdded(server);
	}
}

void ServerFilter::broadcastRemove(ServerItem *server)
{
	for(std::vector<ServerFilterObserver*>::const_iterator it=observers_.begin(); it != observers_.end(); ++it)
	{
			(*it)->notifyServerFilterRemoved(server);
	}
}

void ServerFilter::broadcastChange(ServerItem *server)
{
	for(std::vector<ServerFilterObserver*>::const_iterator it=observers_.begin(); it != observers_.end(); ++it)
	{
			(*it)->notifyServerFilterChanged(server);
	}
}

void ServerFilter::addObserver(ServerFilterObserver* o)
{
	std::vector<ServerFilterObserver*>::iterator it=std::find(observers_.begin(),observers_.end(),o);
	if(it == observers_.end())
	{
		observers_.push_back(o);
	}
}

void ServerFilter::removeObserver(ServerFilterObserver* o)
{
	std::vector<ServerFilterObserver*>::iterator it=std::find(observers_.begin(),observers_.end(),o);
	if(it != observers_.end())
	{
		observers_.erase(it);
	}
}


