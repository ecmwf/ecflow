//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef SERVERFILTER_HPP_
#define SERVERFILTER_HPP_

#include <vector>

#include "Node.hpp"
#include "ServerItem.hpp"
#include "VConfig.hpp"

class ServerHandler;

class ServerFilterItem: public ServerItem
{
	friend class ServerFilter;

public:
	bool hasSuiteFilter();
	const std::vector<node_ptr>&  suiteFilter() const {return suiteFilter_;}
	void addToSuiteFilter(node_ptr);
	void removeFromSuiteFilter(node_ptr);

protected:
	ServerFilterItem(const std::string&,const std::string&,const std::string&);

	ServerHandler* server_;
	std::vector<node_ptr> suiteFilter_;

};


class ServerFilter : public VConfigItem
{
friend class VConfig;

public:
	ServerFilter(VConfig*);

	const std::vector<ServerFilterItem*> servers() const {return servers_;}
	ServerFilterItem* addServer(ServerItem*,bool broadcast=true);
	void removeServer(ServerItem*);
	bool match(ServerItem*);
	void update(const std::vector<ServerItem*>&);

protected:
	void notifyOwner();


private:
	std::vector<ServerFilterItem*> servers_;
};

#endif
