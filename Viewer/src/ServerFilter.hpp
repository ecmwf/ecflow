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

#include <set>
#include <vector>

#include "Node.hpp"
#include "VConfig.hpp"
#include "ServerItem.hpp"

#include <boost/property_tree/ptree.hpp>

/*class ServerFilterItem : public ServerItemObserver
{
	friend class ServerFilter;

public:
	bool hasSuiteFilter();
	const std::set<std::string>&  suiteFilter() const {return suiteFilter_;}
	void addToSuiteFilter(const std::string&);
	void removeFromSuiteFilter(const std::string&);
	ServerHandler* serverHandler() const;

	//From ServerItemObserver
	void notifyServerItemChanged();

protected:
	ServerFilterItem(const std::string&); //,const std::string&,const std::string&);
	~ServerFilterItem();

	ServerItem* server_;
	std::set<std::string> suiteFilter_;
};
*/

class ServerFilter : public VConfigItem, public ServerItemObserver
{
friend class VConfig;

public:
	ServerFilter(VConfig*);
	~ServerFilter();

	const std::vector<ServerItem*> items() const {return items_;}
	void addServer(ServerItem*,bool broadcast=true);
	void removeServer(ServerItem*);
    bool isFiltered(ServerItem*) const;
    void save(boost::property_tree::ptree& array) const;
    void load(const boost::property_tree::ptree& array);

    //From ServerItemObserver
    void notifyServerItemChanged(ServerItem*);
    void notifyServerItemDeletion(ServerItem*);

protected:
	void notifyOwner();

private:
	std::vector<ServerItem*> items_;
};

#endif
