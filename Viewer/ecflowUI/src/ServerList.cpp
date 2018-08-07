//============================================================================
// Copyright 2009-2017 ECMWF.
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
#include "UIDebug.hpp"
#include "UiLog.hpp"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <cstdlib>

#include <boost/lexical_cast.hpp>

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

ServerItem* ServerList::add(const std::string& name,const std::string& host,
                            const std::string& port,bool favourite,bool saveIt)
{
    std::string errStr;
    if(!checkItemToAdd(name,host,port,true,errStr))
    {
        throw std::runtime_error(errStr);
        return 0;
    }

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

bool ServerList::checkItemToAdd(const std::string& name,const std::string& host,const std::string& port,bool checkDuplicate,std::string& errStr)
{
    if(name.empty())
    {
        errStr="Empty server name";
        return false;
    }
    else if(host.empty())
    {
        errStr="Empty server host";
        return false;
    }
    else if(port.empty())
    {
        errStr="Empty server port";
        return false;
    }

    try { boost::lexical_cast<int>(port); }
    catch ( boost::bad_lexical_cast& e)
    {
        errStr="Invalid port number: " + port;
        return false;
    }

    if(checkDuplicate && find(name))
    {
        errStr="Duplicated server name: " + name;
        return false;
    }

    return true;
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
    UiLog().dbg() << "ServerList::load() -->";

    std::ifstream in(localFile_.c_str());
	if(!in.good())
		return false;

    std::string errStr;

	std::string line;
    int lineCnt=1;
	while(getline(in,line))
	{
		//We ignore comment lines
		std::string buf=boost::trim_left_copy(line);
		if(buf.size() > 0 && buf.at(0) == '#')
        {
            lineCnt++;
            continue;
        }

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
            std::string name=sv[0], host=sv[1], port=sv[2];
            ServerItem* item=0;
            try
            {
                item=add(name,host,port,favourite,false);
                UI_ASSERT(item != 0,"name=" << name << " host=" << host << " port=" << port);
                item->setSystem(sys);
            }
            catch(std::exception& e)
            {
                std::string err=e.what();
                err+=" [name=" + name + ",host=" + host + ",port=" + port + "]";
                errStr+=err + " (in line " + UiLog::toString(lineCnt) + ")<br>";
                UiLog().err() << "  " << err << " (in line " << lineCnt << ")";
            }
		}

        lineCnt++;
	}

	in.close();

    if(!errStr.empty())
    {
        errStr="<b>Could not parse</b> the server list file <u>" + localFile_ + "</u>. The \
                    following errors occured:<br><br>" +
                errStr + "<br>Please <b>correct the errors</b> in the server list file and restart ecFlowUI!";
        UserMessage::setEchoToCout(false);
        UserMessage::message(UserMessage::ERROR,true,errStr);
        UserMessage::setEchoToCout(true);
        exit(1);
    }

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

	for(auto & item : items_)
	{
        std::string fav=(item->isFavourite())?"1":"0";
        std::string sys=(item->isSystem())?"1":"0";
        out << item->name() << "," << item->host() << "," <<  item->port() <<  "," <<  fav << "," <<  sys << std::endl;
	}
	out.close();    
}

bool ServerList::readRcFile()
{
    UiLog().dbg() << "ServerList::readRcFile -->";
    std::string path(DirectoryHandler::concatenate(DirectoryHandler::rcDir(), "servers"));

    UiLog().dbg() << " Read servers from ecflowview rcfile: " << path;
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
                std::string name=vec[0], host=vec[1], port=vec[2];                
                try
                {
                    add(name,host,port,false,false);
                }
                catch(std::exception& e)
                {
                    std::string err=e.what();
                    UiLog().err() << " Failed to read server (name=" << name << ",host=" << host <<
                                     ",port=" << port << "). " << err;
                }
			}
		}
	}
	else
		return false;

	in.close();

	return true;
}

bool ServerList::hasSystemFile() const
{
    boost::filesystem::path p(systemFile_);
    return boost::filesystem::exists(p);
}

void ServerList::syncSystemFile()
{
    UiLog().dbg() << "ServerList::syncSystemFile -->";

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
                std::string errStr,name=vec[0], host=vec[1], port=vec[2];
                if(checkItemToAdd(name,host,port,false,errStr))
                {
                    sysVec.push_back(ServerListTmpItem(vec[0],vec[1],vec[2]));
                }
            }
        }
    }
    else
        return;

    in.close();

#ifdef _UI_SERVERLIST_DEBUG
    for(auto & i : sysVec)
        UiLog().dbg() << i.name() << "\t" + i.host() << "\t" + i.port();
#endif

    bool changed=false;
    bool needBrodcast=false;

    //See what changed or was added
    for(auto & i : sysVec)
    {
#ifdef _UI_SERVERLIST_DEBUG
        UiLog().dbg() << i.name() << "\t" + i.host() << "\t" + i.port();
#endif
        ServerItem *item=0;

        //There is a server with same name, host and port as in the local list. We
        //mark it as system
        item=find(i.name(),i.host(),i.port());
        if(item)
        {            
            if(!item->isSystem())
            {
#ifdef _UI_SERVERLIST_DEBUG
                UiLog().dbg() << "  already in list (same name, host, port) -> mark as system";
#endif
                changed=true;
                syncChange_.push_back(new ServerListSyncChangeItem(i,i,
                                     ServerListSyncChangeItem::SetSysChange));
                item->setSystem(true);
            }
            continue;
        }

        //There is no server with the same name in the local list
        item=find(i.name());
        if(!item)
        {
#ifdef _UI_SERVERLIST_DEBUG
            UiLog().dbg() << "  name not in list -> import as system";
#endif
            changed=true;
            std::string name=i.name(),host=i.host(), port=i.port();
            try
            {
                item=add(name,host,port,false,false);
                UI_ASSERT(item != 0,"name=" << name << " host=" << host
                          << " port=" << port);
                item->setSystem(true);
                syncChange_.push_back(new ServerListSyncChangeItem(i,i,
                                         ServerListSyncChangeItem::AddedChange));
            }
            catch(std::exception& e)
            {
                std::string err=e.what();
                UiLog().err() << "  Could not sync server (name=" << name << ",host=" << host <<
                                 "port=" << port << "). " << err;
            }
            continue;
        }
        //There is a server with the same name but with different host or/and port
        else
        {
#ifdef _UI_SERVERLIST_DEBUG
            UiLog().dbg() << "  name in list with different port or/and host";
#endif
            changed=true;
            needBrodcast=true;
            assert(item->name() == sysVec[i].name());

            ServerListTmpItem localTmp(item);
            syncChange_.push_back(new ServerListSyncChangeItem(i,localTmp,
                                     ServerListSyncChangeItem::MatchChange));

            item->reset(i.name(),i.host(),i.port());
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
            for(auto & i : sysVec)
            {
                if(i.name() == (*it)->name())
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
    UiLog().dbg() << "<-- syncSystemFile";
#endif
}

void ServerList::clearSyncChange()
{
    for(auto & i : syncChange_)
        delete i;

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
