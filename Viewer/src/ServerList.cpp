//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "ServerList.hpp"

#include <fstream>
#include <sstream>
#include <stdlib.h>

#include "ServerItem.hpp"

ServerList* ServerList::instance_=0;

ServerList::ServerList()
{
	readRcFile();
}


ServerList* ServerList::Instance()
{
	if(!instance_)
		instance_=new ServerList();
	return instance_;
}


ServerItem* ServerList::item(int index)
{
	return (index >=0 && index < static_cast<int>(items_.size()))?items_.at(index):0;
}

ServerItem* ServerList::add(const std::string& name,const std::string& host,const std::string& port)
{
	ServerItem* item=new ServerItem(name,host,port);
	items_.push_back(item);
	return item;
}

ServerItem* ServerList::add(const std::string& name)
{
	ServerItem* item=new ServerItem(name);
	items_.push_back(item);
	return item;
}

void ServerList::remove(ServerItem* item)
{
}

void ServerList::rescan()
{

}

void ServerList::readRcFile()
{
	std::string path;
	if(const char* h=getenv("HOME"))
	{
			path=std::string(h);
	}
	else
			return;

	path+="/.ecflowrc/servers";

	std::ifstream in(path.c_str());

	if(in.good())
	{
		std::string line;
		while(getline(in,line))
		{
			std::string buf;
			std::stringstream ssdata(line);
			std::vector<std::string> vec;

			while(ssdata >> buf)
			{
				vec.push_back(buf);
			}

			if(vec.size() >= 3)
			{
					add(vec[0],vec[1],vec[2]);
			}
		}
	}

	in.close();
}

void ServerList::readSystemFile()
{

}
