//============================================================================
// Copyright 2015 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "SessionHandler.hpp"

#include <algorithm>

#include "DirectoryHandler.hpp"


SessionHandler* SessionHandler::instance_=0;


SessionItem::SessionItem(const std::string& name) :
  name_(name)
{
	checkDir();
}

void SessionItem::checkDir()
{
	dirPath_=DirectoryHandler::concatenate(DirectoryHandler::configDir(), name_ + ".session");
	DirectoryHandler::createDir(dirPath_);

}

std::string SessionItem::sessionFile() const
{
	return DirectoryHandler::concatenate(dirPath_, "session.json");
}

std::string SessionItem::windowFile() const
{
	return DirectoryHandler::concatenate(dirPath_, "window.conf");
}

std::string SessionItem::settingsFile() const
{
	return DirectoryHandler::concatenate(dirPath_, "settings.json");
}

std::string SessionItem::recentCustomCommandsFile() const
{
    return DirectoryHandler::concatenate(dirPath_, "recent_custom_commands.json");
}

std::string SessionItem::savedCustomCommandsFile() const
{
    return DirectoryHandler::concatenate(dirPath_, "saved_custom_commands.json");
}

std::string SessionItem::serverFile(const std::string& serverName) const
{
	return DirectoryHandler::concatenate(dirPath_, serverName + ".conf.json");
}


//=================================================
//
// SessionHandler
//
//=================================================

SessionHandler::SessionHandler() :
	current_(0)
{
	//The default must always be exist!
	current_=add("default");
}

SessionHandler* SessionHandler::instance()
{
	if(!instance_)
	{
		instance_=new SessionHandler();
	}

	return instance_;
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
	if(std::find(sessions_.begin(),sessions_.end(),item) != sessions_.end())
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
		//current_->save();
	}
}

void SessionHandler::load()
{

}

