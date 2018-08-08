//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "ServerItem.hpp"
#include "ServerHandler.hpp"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <cstdlib>


ServerItem::ServerItem(const std::string& name) :
  name_(name),
  favourite_(false),
  system_(false),
  useCnt_(0),
  handler_(nullptr)
{
}

ServerItem::ServerItem(const std::string& name,const std::string& host,const std::string& port, bool favourite) :
  name_(name), host_(host), port_(port),
  favourite_(favourite),
  system_(false),
  useCnt_(0),
  handler_(nullptr)
{
}

ServerItem::~ServerItem()
{
	broadcastDeletion();

	if(handler_)
		ServerHandler::removeServer(handler_);
}

bool ServerItem::isUsed() const
{
	return (handler_ != nullptr);
}


void ServerItem::reset(const std::string& name,const std::string& host,const std::string& port)
{
	name_=name;
	host_=host;
	port_=port;

	broadcastChanged();
}

void ServerItem::setFavourite(bool b)
{
	favourite_=b;
	broadcastChanged();
}

void ServerItem::setSystem(bool b)
{
    system_=b;
    //broadcastChanged();
}

std::string ServerItem::longName() const
{
    return host_ + "@" + port_;
}

//===========================================================
// Register the usage of the server. Create and destroys the
// the ServerHandler.
//===========================================================

void ServerItem::registerUsageBegin()
{
	if(!handler_)
	{
		handler_=ServerHandler::addServer(name_,host_,port_);
    }
    if(handler_)
        useCnt_++;
}

void ServerItem::registerUsageEnd()
{
	useCnt_--;
	if(useCnt_ == 0 && handler_)
	{
		ServerHandler::removeServer(handler_);
		handler_=nullptr;
	}
}

//===========================================================
// Observers
//===========================================================

void ServerItem::addObserver(ServerItemObserver* o)
{
	auto it=std::find(observers_.begin(),observers_.end(),o);
	if(it == observers_.end())
	{	
		registerUsageBegin();
        //We might not be able to create the handle
        if(handler_)
            observers_.push_back(o);
	}
}

void ServerItem::removeObserver(ServerItemObserver* o)
{
	auto it=std::find(observers_.begin(),observers_.end(),o);
	if(it != observers_.end())
	{
		observers_.erase(it);
		registerUsageEnd();
	}
}

void ServerItem::broadcastChanged()
{
	for(std::vector<ServerItemObserver*>::const_iterator it=observers_.begin(); it != observers_.end(); ++it)
		(*it)->notifyServerItemChanged(this);
}

void ServerItem::broadcastDeletion()
{
	std::vector<ServerItemObserver*> obsCopy=observers_;

	for(std::vector<ServerItemObserver*>::const_iterator it=obsCopy.begin(); it != obsCopy.end(); ++it)
		(*it)->notifyServerItemDeletion(this);
}
