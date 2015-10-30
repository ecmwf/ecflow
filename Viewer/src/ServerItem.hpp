//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef SERVERITEM_HPP_
#define SERVERITEM_HPP_

#include <set>
#include <string>
#include <vector>

#include <cstddef>
#include <boost/shared_ptr.hpp>

class ServerHandler;
class ServerItem;

class ServerItemObserver
{
public:
	ServerItemObserver() {};
	virtual ~ServerItemObserver() {};
	virtual void notifyServerItemChanged(ServerItem*)=0;
	virtual void notifyServerItemDeletion(ServerItem*)=0;
};


class ServerItem
{

friend class ServerList;

public:
	const std::string& name() const {return name_;}
	const std::string& host() const {return host_;}
	const std::string& port() const {return port_;}
	bool isFavourite() const {return favourite_;}

	bool isUsed() const;
	int useCnt() const {return useCnt_;}
    ServerHandler* serverHandler() const {return handler_;}

	void addObserver(ServerItemObserver*);
	void removeObserver(ServerItemObserver*);

protected:
	explicit ServerItem(const std::string&);
	ServerItem(const std::string&,const std::string&,const std::string&,bool favourite=false);
	 ~ServerItem();

	void  name(const std::string& name) {name_=name;}
	void  host(const std::string& host) {host_=host;}
	void  port(const std::string& port) {port_=port;}
	void  reset(const std::string& name,const std::string& host,const std::string& port);
	void  setFavourite(bool b);

	void registerUsageBegin();
	void registerUsageEnd();

	void broadcastChanged();
	void broadcastDeletion();

	std::string name_;
	std::string host_;
	std::string port_;
	bool favourite_;
	int useCnt_;
	ServerHandler* handler_;

	std::set<std::string> suiteFilter_;
	std::vector<ServerItemObserver*> observers_;
};

typedef boost::shared_ptr<ServerItem>   ServerItem_ptr;

#endif
