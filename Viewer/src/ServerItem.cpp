//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "ServerItem.hpp"

ServerItem::ServerItem(const std::string& name) :
  name_(name)
{
}

ServerItem::ServerItem(const std::string& name,const std::string& host,const std::string& port) :
  name_(name), host_(host), port_(port)
{
}

ServerItem* ServerItem::clone() const
{
	return new ServerItem(name_,host_,port_);
}

bool ServerItem::match(ServerItem* item)
{
	if(!item)
		return false;

	if(host_ == item->host() && port_ == item->port())
	{
		return true;
	}
	return false;
}


/*
ServerViewItem::ServerViewItem(const std::string& name,const std::string& host,const std::string& port) :
		ServerItem(name,host,port)
{

}

bool ServerViewItem::hasSuiteFilter()
{
	return (suiteFilter_.size()>0);
}

void ServerViewItem::addToSuiteFilter(node_ptr suite)
{
	suiteFilter_.push_back(suite);
}

void ServerViewItem::removeFromSuiteFilter(node_ptr suite)
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
*/
