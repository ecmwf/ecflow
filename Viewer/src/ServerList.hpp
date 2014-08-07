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


class ServerList
{
public:
	static ServerList* Instance();

	int count() const {return static_cast<int>(items_.size());}
	ServerItem* item(int);

	void rescan();
	void update(const std::vector<ServerItem*>&);
	void save();

private:
	ServerList();
	~ServerList();

	ServerItem* add(const std::string&,const std::string&,const std::string&);
	ServerItem* add(const std::string&);
	void add(ServerItem*);

	bool load();
	bool readRcFile();
	bool readSystemFile();

	static ServerList* instance_;

	std::vector<ServerItem*> items_;
	std::string serversPath_;

};


#endif
