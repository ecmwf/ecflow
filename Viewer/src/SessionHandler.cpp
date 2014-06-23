//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "SessionHandler.hpp"

#include <algorithm>

SessionItem::SessionItem(const std::string& name) : name_(name)
{

}


void SessionItem::name(const std::string& name)
{
	name_=name;
	//rename file on disk
}


SessionHandler::SessionHandler() : current_(0)
{

}

SessionItem* SessionHandler::add(const std::string& name)
{
	SessionItem *item=new SessionItem(name);
	sessions_.push_back(item);
	return item;
}

void SessionHandler::remove(const std::string&)
{
}

void SessionHandler::remove(SessionItem*)
{

}

void SessionHandler::current(SessionItem* item)
{
	std::vector<SessionItem*>::const_iterator it;

	if((it=std::find(sessions_.begin(),sessions_.end(),item)) != sessions_.end())
	{
		current_=item;
		load();
	}
}


SessionItem* SessionHandler::current()
{
	return current_;
}

void SessionHandler::save()
{
	if(current_)
	{
		current_->save();
	}
}

void SessionHandler::load()
{

}

