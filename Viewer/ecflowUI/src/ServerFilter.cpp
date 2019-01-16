//============================================================================
// Copyright 2009-2019 ECMWF.
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
#include "SessionHandler.hpp"

//==============================================
//
// ServerFilter
//
//==============================================

ServerFilter::ServerFilter()
= default;

ServerFilter::~ServerFilter()
{
	std::vector<ServerFilterObserver*> obsCopy=observers_;
	for(std::vector<ServerFilterObserver*>::const_iterator it=obsCopy.begin(); it != obsCopy.end(); ++it)
	{
		(*it)->notifyServerFilterDelete();
	}

	for(std::vector<ServerItem*>::const_iterator it=items_.begin(); it != items_.end(); ++it)
	{
		(*it)->removeObserver(this);
	}
}

void ServerFilter::serverNames(std::vector<std::string>& vec) const
{
	for(auto item : items_)
	{
		vec.push_back(item->name());
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

	auto it=std::find(items_.begin(),items_.end(),server);
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

	auto it=std::find(items_.begin(),items_.end(),server);
	if(it != items_.end())
	{
		items_.erase(it);

		//Notifies the view about the changes
		broadcastRemove(server);
	}
}

bool ServerFilter::isFiltered(ServerItem* item) const
{
	for(auto it : items_)
	{
		if(it == item)
			return true;
	}
	return false;
}

bool ServerFilter::isFiltered(ServerHandler* server) const
{
    for(auto item : items_)
    {
        if(item->serverHandler() == server)
            return true;
    }
    return false;
}

void ServerFilter::writeSettings(VSettings* vs) const
{
	std::vector<std::string> array;
	for(auto item : items_)
	{
		array.push_back(item->name());
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
			addServer(s,true);
		}
		// special case - if we're starting a temporary session for looking at one
		// particular server, then we need to replace the placeholder server alias
		// with the one we actually want to look at
		else if (name == "SERVER_ALIAS_PLACEHOLDER")
		{
			SessionItem *session = SessionHandler::instance()->current();
			if (session->temporary())
			{
				std::string alias = session->temporaryServerAlias();
				if (ServerItem* s=ServerList::instance()->find(alias))
				{
					addServer(s,true);
				}
			}
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
	auto it=std::find(observers_.begin(),observers_.end(),o);
	if(it == observers_.end())
	{
		observers_.push_back(o);
	}
}

void ServerFilter::removeObserver(ServerFilterObserver* o)
{
	auto it=std::find(observers_.begin(),observers_.end(),o);
	if(it != observers_.end())
	{
		observers_.erase(it);
	}
}


