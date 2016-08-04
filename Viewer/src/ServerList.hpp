//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef SERVERLIST_HPP_
#define SERVERLIST_HPP_

#include <string>
#include <vector>

class ServerItem;
class ServerList;

class ServerListObserver
{
public:
	ServerListObserver() {};
	virtual ~ServerListObserver() {};
	virtual void notifyServerListChanged()=0;
	virtual void notifyServerListFavouriteChanged(ServerItem*)=0;
};

class ServerList
{
public:
	int count() {return static_cast<int>(items_.size());}
	ServerItem* itemAt(int);
	ServerItem* find(const std::string& name);
	ServerItem* find(const std::string& name, const std::string& host, const std::string& port);

	//Can be added or changed only via these static methods
	ServerItem* add(const std::string&,const std::string&,const std::string&,bool,bool saveIt=true);
	void remove(ServerItem*);
	void reset(ServerItem*,const std::string& name,const std::string& host,const std::string& port);
	void setFavourite(ServerItem*,bool);

	std::string uniqueName(const std::string&);

	void init();
	void save();
	void rescan();

	void addObserver(ServerListObserver*);
	void removeObserver(ServerListObserver*);

	static ServerList* instance();

protected:
	ServerList() {};
	~ServerList() {};

	static ServerList* instance_;

	bool load();
	bool readRcFile();
	bool readSystemFile();

	void broadcastChanged();
	void broadcastChanged(ServerItem*);

	std::vector<ServerItem*> items_;
	std::string serversPath_;
	std::vector<ServerListObserver*> observers_;
};



#endif
