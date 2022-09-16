//============================================================================
// Copyright 2009- ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include <stdexcept>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

#include "ServerList.hpp"

#include "Str.hpp"
#include "DirectoryHandler.hpp"
#include "ServerItem.hpp"
#include "UserMessage.hpp"
#include "UIDebug.hpp"
#include "UiLog.hpp"
#include "VReply.hpp"
#include "MainWindow.hpp"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <cstdlib>

#include <boost/lexical_cast.hpp>

ServerList* ServerList::instance_=nullptr;

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
	return (index >=0 && index < static_cast<int>(items_.size()))?items_.at(index):nullptr;
}

ServerItem* ServerList::find(const std::string& name)
{
	for(std::vector<ServerItem*>::const_iterator it=items_.begin(); it != items_.end(); ++it)
	{
		if((*it)->name() == name)
			return *it;
	}
	return nullptr;
}

ServerItem* ServerList::find(const std::string& name, const std::string& host, const std::string& port)
{
	for(std::vector<ServerItem*>::const_iterator it=items_.begin(); it != items_.end(); ++it)
	{
		if((*it)->name() == name && (*it)->host() == host && (*it)->port() == port)
			return *it;
	}
	return nullptr;
}

ServerItem* ServerList::add(const std::string& name,const std::string& host,
                            const std::string& port,const std::string& user,
                            bool favourite, bool ssl, bool saveIt)
{
    std::string errStr;
    if(!checkItemToAdd(name,host,port,true,errStr))
    {
        throw std::runtime_error(errStr);
        return nullptr;
    }

    auto* item=new ServerItem(name,host,port,user,favourite,ssl);

    items_.push_back(item);

	if(saveIt)
		save();

	broadcastChanged();

	return item;
}

void ServerList::remove(ServerItem *item)
{
	auto it=std::find(items_.begin(),items_.end(),item);
	if(it != items_.end())
	{
		items_.erase(it);
		//item->broadcastDeletion();
		delete item;

		save();
		broadcastChanged();
	}
}

ServerItem* ServerList::reset(ServerItem* item,const std::string& name,const std::string& host,
                       const std::string& port, const std::string& user, bool ssl)
{
	auto it=std::find(items_.begin(),items_.end(),item);
	if(it != items_.end())
	{
        //Check if there is an item with the same name. Names have to be unique!
		if(item->name() != name && find(name))
            return nullptr;

        if (host != item->host() || port != item->port()) {
            items_.erase(it);
            broadcastChanged();
            delete item;
            item = add(name,host,port,user,false,ssl,true);
            save();
            broadcastChanged();
        } else {
            assert(host == item->host());
            assert(port == item->port());
            item->reset(name,host,port,user,ssl);
            save();
            broadcastChanged();
        }
	}
    return item;
}

void ServerList::setFavourite(ServerItem* item,bool b)
{
	auto it=std::find(items_.begin(),items_.end(),item);
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
    if (char *ch = getenv("ECFLOW_SERVERS_LIST")) {
        auto s = std::string(ch);
        ecf::Str::split(s, systemFiles_ ,":");
        for (auto p: systemFiles_) {
            UiLog().dbg() << UI_FN_INFO <<  p;
        }
    }

    //systemFile_=DirectoryHandler::concatenate(DirectoryHandler::shareDir(), "servers");

    if(load() == false)
	{		
        if(readRcFile())
            save();
	}

    //syncSystemFiles();
}

bool ServerList::load()
{
    UiLog().dbg() << UI_FN_INFO << "-->";

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

        bool ssl=false;
        if(sv.size() >= 6)
            ssl=(sv[5]=="1")?true:false;

        std::string user;
        if(sv.size() >= 7)
            user=sv[6];

        if(sv.size() >= 3)
		{           
            std::string name=sv[0], host=sv[1], port=sv[2];
            ServerItem* item=nullptr;
            try
            {
                item=add(name,host,port,user,favourite,ssl,false);
                UI_ASSERT(item != nullptr,"name=" << name << " host=" << host << " port=" << port << " user=" << user);
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

    out << "#Name Host Port Favourite System Ssl user" << std::endl;

	for(auto & item : items_)
	{
        std::string fav=(item->isFavourite())?"1":"0";
        std::string ssl=(item->isSsl())?"1":"0";
        std::string sys=(item->isSystem())?"1":"0";
        out << item->name() << "," << item->host() << "," <<  item->port() <<  "," <<  fav << "," <<  sys << "," <<  ssl << "," << item->user() << std::endl;
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
                    add(name,host,port,"",false,false,false);
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
    return hasSystemFiles_;
}

void ServerList::syncSystemFiles()
{
    if (!systemFiles_.empty()) {
        fetchManager_->fetchFiles(systemFiles_);
    }
}

void ServerList::syncSystemFiles(const std::vector<std::string>& paths)
{
    std::vector<ServerListTmpItem> sysVec;
    for (auto f: paths) {
        readSystemFile(f, sysVec);
    }

    // nothing to sync
    if (sysVec.empty()) {
        return;
    }

    hasSystemFiles_ = true;

    bool changed=false;
    bool needBrodcast=false;

#ifdef _UI_SERVERLIST_DEBUG
        UiLog().dbg() << UI_FN_INFO << "Load system server list:";
#endif

    //See what changed or was added
    for(auto & i : sysVec)
    {
#ifdef _UI_SERVERLIST_DEBUG
        UiLog().dbg() << i.name() << " " + i.host() << " " + i.port();
#endif
        ServerItem *item=nullptr;

        //There is a server with the same name, host and port as in the local list. We
        //mark it as system
        item=find(i.name(),i.host(),i.port());
        if(item)
        {
            if(!item->isSystem())
            {
#ifdef _UI_SERVERLIST_DEBUG
                UiLog().dbg() << " -> already in server-list (same name, host, port). Mark as system server";
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
            UiLog().dbg() << "  -> name is not in server-list. Import as system server";
#endif
            changed=true;
            std::string name=i.name(),host=i.host(), port=i.port();
            try
            {
                item=add(name,host,port,"",false,false,false);
                UI_ASSERT(item != nullptr,"name=" << name << " host=" << host
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
            UiLog().dbg() << "  -> name exsist in server-list with different port or/and host: " << item->host() << "@" << item->port() <<
            " ! Reset host and port";
#endif
            changed=true;
            needBrodcast=true;
            //assert(item->name() == sysVec[i].name());

            ServerListTmpItem localTmp(item);
            syncChange_.push_back(new ServerListSyncChangeItem(i,localTmp,
                                     ServerListSyncChangeItem::MatchChange));

            item = reset(item, i.name(),i.host(),i.port(), "",false);
            if (item) {
                item->setSystem(true);
            }
            //item->reset(i.name(),i.host(),i.port(), "",false);
            //broadcastChanged();
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
}

void ServerList::readSystemFile(const std::string& fPath, std::vector<ServerListTmpItem>& sysVec)
{
    std::ifstream in(fPath.c_str());

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
                    sysVec.emplace_back(vec[0],vec[1],vec[2]);
                }
            }
        }
    }
    in.close();
}

void ServerList::clearSyncChange()
{
    for(auto & i : syncChange_) {
        delete i;
    }
    syncChange_.clear();
}


//===========================================================
// Observers
//===========================================================

void ServerList::addObserver(ServerListObserver* o)
{
	auto it=std::find(observers_.begin(),observers_.end(),o);
	if(it == observers_.end())
	{
		observers_.push_back(o);
	}
}

void ServerList::removeObserver(ServerListObserver* o)
{
	auto it=std::find(observers_.begin(),observers_.end(),o);
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

void ServerList::fileFetchFinished(VReply* r)
{
    if (!r->tmpFiles().empty()) {
        syncDate_=QDateTime::currentDateTime();
        clearSyncChange();
        std::vector<std::string> paths;
        for (auto f: r->tmpFiles()) {
            //UiLog().dbg() << "f=" << f->path();
            if (f) {
                paths.emplace_back(f->path());
            }
        }
        syncSystemFiles(paths);
        MainWindow::initServerSyncTb();
    }
}

void ServerList::fileFetchFailed(VReply* /*r*/)
{
}
