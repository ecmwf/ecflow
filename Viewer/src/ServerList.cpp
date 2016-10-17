//============================================================================
// Copyright 2016 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include <boost/algorithm/string.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

#include "ServerList.hpp"

#include "DirectoryHandler.hpp"
#include "ServerItem.hpp"
#include "UserMessage.hpp"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdlib.h>

ServerList* ServerList::instance_=0;

#define _UI_SERVERLIST_DEBUG

ServerListTmpItem::ServerListTmpItem(ServerItem* item) :
    name_(item->name()),
    host_(item->host()),
    port_(item->port())
{}

//Singleton instance method
ServerList* ServerList::instance()
{
	if(!instance_)
			instance_=new ServerList();
	return instance_;
}

//===========================================================
// Managing server items
//===========================================================

ServerItem* ServerList::itemAt(int index)
{
	return (index >=0 && index < static_cast<int>(items_.size()))?items_.at(index):0;
}

ServerItem* ServerList::find(const std::string& name)
{
	for(std::vector<ServerItem*>::const_iterator it=items_.begin(); it != items_.end(); ++it)
	{
		if((*it)->name() == name)
			return *it;
	}
	return 0;
}

ServerItem* ServerList::find(const std::string& name, const std::string& host, const std::string& port)
{
	for(std::vector<ServerItem*>::const_iterator it=items_.begin(); it != items_.end(); ++it)
	{
		if((*it)->name() == name && (*it)->host() == host && (*it)->port() == port)
			return *it;
	}
	return 0;
}

ServerItem* ServerList::add(const std::string& name,const std::string& host,const std::string& port,bool favourite,bool saveIt)
{
    //Check if there is an item with the same name. Names have to be unique!
	if(find(name))
		return 0;

	ServerItem* item=new ServerItem(name,host,port,favourite);
	items_.push_back(item);

	if(saveIt)
		save();

	broadcastChanged();

	return item;
}

void ServerList::remove(ServerItem *item)
{
	std::vector<ServerItem*>::iterator it=std::find(items_.begin(),items_.end(),item);
	if(it != items_.end())
	{
		items_.erase(it);
		//item->broadcastDeletion();
		delete item;

		save();
		broadcastChanged();
	}
}

void ServerList::reset(ServerItem* item,const std::string& name,const std::string& host,const std::string& port)
{
	std::vector<ServerItem*>::iterator it=std::find(items_.begin(),items_.end(),item);
	if(it != items_.end())
	{
		//Check if there is an item with the same name. names have to be unique!
		if(item->name() != name && find(name))
			return;

		item->reset(name,host,port);

		save();
		broadcastChanged();
	}
}

void ServerList::setFavourite(ServerItem* item,bool b)
{
	std::vector<ServerItem*>::iterator it=std::find(items_.begin(),items_.end(),item);
	if(it != items_.end())
	{
		item->setFavourite(b);
		for(std::vector<ServerListObserver*>::const_iterator it=observers_.begin(); it != observers_.end(); ++it)
			(*it)->notifyServerListFavouriteChanged(item);
	}
}

std::string ServerList::uniqueName(const std::string& name)
{
	bool hasIt=false;
    for(std::vector<ServerItem*>::const_iterator it=items_.begin(); it != items_.end(); ++it)
    {
        if((*it)->name() == name)
        {
            hasIt=true;
            break;
        }
    }
	if(!hasIt)
	{
		return name;
	}

	for(int i=1; i < 100; i++)
	{
		std::ostringstream c;
		c << i;
		std::string currentName=name+"_"+c.str();

		hasIt=false;
		for(std::vector<ServerItem*>::const_iterator it=items_.begin(); it != items_.end(); ++it)
		{
			if((*it)->name() == currentName)
			{
				hasIt=true;
				break;
			}
		}
		if(!hasIt)
		{
			return currentName;
		}
	}

	return name;

}


void ServerList::rescan()
{

}

//===========================================================
// Initialisation
//===========================================================

void ServerList::init()
{
    localFile_ = DirectoryHandler::concatenate(DirectoryHandler::configDir(), "servers.txt");
    systemFile_=DirectoryHandler::concatenate(DirectoryHandler::shareDir(), "servers");

    if(load() == false)
	{		
        if(readRcFile())
            save();
	}

    syncSystemFile();
}

bool ServerList::load()
{
    std::ifstream in(localFile_.c_str());
	if(!in.good())
		return false;

	std::string line;
	while(getline(in,line))
	{
		//We ignore comment lines
		std::string buf=boost::trim_left_copy(line);
		if(buf.size() > 0 && buf.at(0) == '#')
			continue;

		std::vector<std::string> sv;
		boost::split(sv,line,boost::is_any_of(","));

		bool favourite=false;
		if(sv.size() >= 4)
			favourite=(sv[3]=="1")?true:false;

        bool sys=false;
        if(sv.size() >= 5)
            sys=(sv[4]=="1")?true:false;

        if(sv.size() >= 3)
		{
            ServerItem* item=add(sv[0],sv[1],sv[2],favourite,false);
            item->setSystem(sys);
		}
	}

	in.close();

	if(count() == 0)
		return false;

	return true;
}

void ServerList::save()
{
	std::ofstream out;
    out.open(localFile_.c_str());
	if(!out.good())
		  	return;

    out << "#Name Host Port Favourite System" << std::endl;

	for(std::vector<ServerItem*>::iterator it=items_.begin(); it != items_.end(); ++it)
	{
        std::string fav=((*it)->isFavourite())?"1":"0";
        std::string sys=((*it)->isSystem())?"1":"0";
        out << (*it)->name() << "," << (*it)->host() << "," <<  (*it)->port() <<  "," <<  fav << "," <<  sys << std::endl;
	}
	out.close();    
}


bool ServerList::readRcFile()
{
	std::string path(DirectoryHandler::concatenate(DirectoryHandler::rcDir(), "servers"));

	std::ifstream in(path.c_str());

	if(in.good())
	{
		std::string line;
		while(getline(in,line))
		{
			std::string buf=boost::trim_left_copy(line);
			if(buf.size() > 0 && buf.at(0) == '#')
				continue;

			std::stringstream ssdata(line);
			std::vector<std::string> vec;

			while(ssdata >> buf)
			{
				vec.push_back(buf);
			}

			if(vec.size() >= 3)
			{
				add(vec[0],vec[1],vec[2],false,false);
			}
		}
	}
	else
		return false;

	in.close();

	return true;
}

#if 0
bool ServerList::readSystemFile()
{
	std::string path(DirectoryHandler::concatenate(DirectoryHandler::shareDir(), "servers"));
    std::ifstream in(path.c_str());

	if(in.good())
	{
		std::string line;
		while(getline(in,line))
		{
			std::string buf=boost::trim_left_copy(line);
			if(buf.size() >0 && buf.at(0) == '#')
					continue;

			std::stringstream ssdata(line);
			std::vector<std::string> vec;

			while(ssdata >> buf)
			{
				vec.push_back(buf);
			}

			if(vec.size() >= 3)
			{
                ServerItem *item=add(vec[0],vec[1],vec[2],false,false);
                item->setSystem(true);
			}
		}
	}
	else
		return false;

	in.close();

	return true;
}
#endif


bool ServerList::hasSystemFile() const
{
    boost::filesystem::path p(systemFile_);
    return boost::filesystem::exists(p);
}

void ServerList::syncSystemFile()
{
#ifdef _UI_SERVERLIST_DEBUG
    UserMessage::debug("ServerList::syncSystemFile -->");
#endif

    std::vector<ServerListTmpItem> sysVec;
    std::ifstream in(systemFile_.c_str());

    syncDate_=QDateTime::currentDateTime();
    clearSyncChange();

    if(in.good())
    {
        std::string line;
        while(getline(in,line))
        {
            std::string buf=boost::trim_left_copy(line);
            if(buf.size() >0 && buf.at(0) == '#')
                    continue;

            std::stringstream ssdata(line);
            std::vector<std::string> vec;

            while(ssdata >> buf)
            {
                vec.push_back(buf);
            }

            if(vec.size() >= 3)
            {
                sysVec.push_back(ServerListTmpItem(vec[0],vec[1],vec[2]));
            }
        }
    }
    else
        return;

    in.close();

#ifdef _UI_SERVERLIST_DEBUG
    for(unsigned int i=0; i < sysVec.size(); i++)
        UserMessage::debug(sysVec[i].name() + "\t" + sysVec[i].host() + "\t" + sysVec[i].port());
#endif

    bool changed=false;
    bool needBrodcast=false;

    //See what changed or was added
    for(unsigned int i=0; i < sysVec.size(); i++)
    {
#ifdef _UI_SERVERLIST_DEBUG
        UserMessage::debug(sysVec[i].name() + "\t" + sysVec[i].host() + "\t" + sysVec[i].port());
#endif
        ServerItem *item=0;

        //There is a server with same name, host and port as in the local list. We
        //mark it as system
        item=find(sysVec[i].name(),sysVec[i].host(),sysVec[i].port());
        if(item)
        {            
            if(!item->isSystem())
            {
#ifdef _UI_SERVERLIST_DEBUG
                UserMessage::debug("  already in list (same name, host, port) -> mark as system");
#endif
                changed=true;
                syncChange_.push_back(new ServerListSyncChangeItem(sysVec[i],sysVec[i],
                                     ServerListSyncChangeItem::SetSysChange));
                item->setSystem(true);
            }
            continue;
        }

        //There is no server with the same name in the local list
        item=find(sysVec[i].name());
        if(!item)
        {
#ifdef _UI_SERVERLIST_DEBUG
            UserMessage::debug("  name not in list -> import as system");
#endif
            changed=true;
            item=add(sysVec[i].name(),sysVec[i].host(),sysVec[i].port(), false, false);
            item->setSystem(true);
            syncChange_.push_back(new ServerListSyncChangeItem(sysVec[i],sysVec[i],
                                     ServerListSyncChangeItem::AddedChange));
            continue;
        }
        //There is a server with the same name but with different host or/and port
        else
        {
#ifdef _UI_SERVERLIST_DEBUG
            UserMessage::debug("  name in list with different port or/and host");
#endif
            changed=true;
            needBrodcast=true;
            assert(item->name() == sysVec[i].name());

            ServerListTmpItem localTmp(item);
            syncChange_.push_back(new ServerListSyncChangeItem(sysVec[i],localTmp,
                                     ServerListSyncChangeItem::MatchChange));

            item->reset(sysVec[i].name(),sysVec[i].host(),sysVec[i].port());
            item->setSystem(true);
            broadcastChanged();
            continue;
        }
    }

    std::vector<ServerItem*> itemsCopy=items_;

    //See what needs to be removed
    for(std::vector<ServerItem*>::const_iterator it=itemsCopy.begin(); it != itemsCopy.end(); ++it)
    {
        if((*it)->isSystem())
        {
            bool found=false;
            for(unsigned int i=0; i < sysVec.size(); i++)
            {
                if(sysVec[i].name() == (*it)->name())
                {
                    found=true;
                    break;
                }
            }
            if(!found)
            {
                changed=true;
                ServerListTmpItem localTmp(*it);
                syncChange_.push_back(new ServerListSyncChangeItem(localTmp,localTmp,
                                  ServerListSyncChangeItem::UnsetSysChange));
                //remove item
                remove(*it);
            }
         }
     }

    if(changed)
        save();

    if(needBrodcast)
        broadcastChanged();

#ifdef _UI_SERVERLIST_DEBUG
    UserMessage::debug("<-- ServerList::syncSystemFile");
#endif
}

void ServerList::clearSyncChange()
{
    for(size_t i=0;i < syncChange_.size(); i++)
        delete syncChange_[i];

    syncChange_.clear();
}


//===========================================================
// Observers
//===========================================================

void ServerList::addObserver(ServerListObserver* o)
{
	std::vector<ServerListObserver*>::iterator it=std::find(observers_.begin(),observers_.end(),o);
	if(it == observers_.end())
	{
		observers_.push_back(o);
	}
}

void ServerList::removeObserver(ServerListObserver* o)
{
	std::vector<ServerListObserver*>::iterator it=std::find(observers_.begin(),observers_.end(),o);
	if(it != observers_.end())
	{
		observers_.erase(it);
	}
}

void ServerList::broadcastChanged()
{
	for(std::vector<ServerListObserver*>::const_iterator it=observers_.begin(); it != observers_.end(); ++it)
		(*it)->notifyServerListChanged();
}
