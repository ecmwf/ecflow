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


ServerFilterItem::ServerFilterItem(const std::string& name,const std::string& host,const std::string& port) :
		ServerItem(name,host,port),
		server_(0)
{
	server_=ServerHandler::find(host,port);
	if(!server_)
		server_=ServerHandler::addServer(host,port);

}

bool ServerFilterItem::hasSuiteFilter()
{
	return (suiteFilter_.size()>0);
}

void ServerFilterItem::addToSuiteFilter(node_ptr suite)
{
	suiteFilter_.push_back(suite);
}

void ServerFilterItem::removeFromSuiteFilter(node_ptr suite)
{
	for(std::vector<node_ptr>::iterator it=suiteFilter_.begin(); it != suiteFilter_.end(); it++)
	{
		if((*it) == suite)
		{
				suiteFilter_.erase(it);
				return;
		}

	}
}


//==============================================
//
// ServerFilter
//
//==============================================

ServerFilter::ServerFilter()
{
}

ServerFilterItem* ServerFilter::addServer(ServerItem *item,bool broadcast)
{
	if(item)
	{
			ServerFilterItem* s=new ServerFilterItem(item->name(),item->host(),item->port());
			if(s->server_)
			{
					servers_.push_back(s);
					if(broadcast)
						broadcastChange();
					return s;
			}
			else
				delete s;
	}

	return 0;
}

void ServerFilter::removeServer(ServerItem *item)
{

}

void ServerFilter::broadcastChange()
{
	for(std::vector<ServerFilterObserver*>::iterator it=observers_.begin(); it != observers_.end(); it++)
	{
		(*it)->notifyServerFilterChanged();
	}
}

void ServerFilter::addObserver(ServerFilterObserver* obs)
{
	observers_.push_back(obs);
}

void ServerFilter::removeObserver(ServerFilterObserver* obs)
{
	std::vector<ServerFilterObserver*>::iterator it=std::find(observers_.begin(),observers_.end(),obs);
	if(it != observers_.end())
		observers_.erase(it);
}

bool ServerFilter::match(ServerItem* item)
{
	if(!item)
		return false;

	for(std::vector<ServerFilterItem*>::iterator it=servers_.begin(); it != servers_.end(); it++)
	{
			if((*it)->host() == item->host() && (*it)->port() == item->port())
			{
					return true;
			}
	}
	return false;
}

void  ServerFilter::update(const std::vector<ServerItem*>& items)
{
	bool changed=false;

	//Remove
	for(std::vector<ServerFilterItem*>::iterator it=servers_.begin(); it != servers_.end(); it++)
	{
		bool found=false;
		for(std::vector<ServerItem*>::const_iterator itA=items.begin(); itA != items.end(); itA++)
		{
			if(!(*itA)->match(*it))
			{
				found=true;
				break;
			}
		}
		if(!found)
		{
			changed=true;
		}
	}
	//Add
	for(std::vector<ServerItem*>::const_iterator itA=items.begin(); itA != items.end(); itA++)
	{
		bool found=false;
		for(std::vector<ServerFilterItem*>::iterator it=servers_.begin(); it != servers_.end(); it++)
		{
			if((*itA)->match(*it))
			{
					found=true;
					break;
			}
		}
		if(!found)
		{
			if(ServerFilterItem *sf=addServer(*itA,false))
			{
				changed=true;
			}
		}
	}

	if(changed)
		broadcastChange();

}



