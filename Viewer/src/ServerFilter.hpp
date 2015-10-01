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

class VSettings;

#include <boost/property_tree/ptree.hpp>

class ServerFilterObserver
{
public:
	virtual ~ServerFilterObserver() {};
	virtual void notifyServerFilterAdded(ServerItem*)=0;
	virtual void notifyServerFilterRemoved(ServerItem*)=0;
	virtual void notifyServerFilterChanged(ServerItem*)=0;
	virtual void notifyServerFilterDelete()=0;
};

class ServerFilter : public ServerItemObserver
{
public:
	ServerFilter();
	~ServerFilter();

	enum ChangeAspect {Reset,Added,Removed};

	const std::vector<ServerItem*>& items() const {return items_;}
	void addServer(ServerItem*,bool broadcast=true);
	void removeServer(ServerItem*);
    bool isFiltered(ServerItem*) const;

    void writeSettings(VSettings*) const;
    void readSettings(VSettings*);

    void addObserver(ServerFilterObserver*);
    void removeObserver(ServerFilterObserver*);

    //From ServerItemObserver
    void notifyServerItemChanged(ServerItem*);
    void notifyServerItemDeletion(ServerItem*);

protected:
    void broadcastAdd(ServerItem*);
    void broadcastRemove(ServerItem*);
    void broadcastChange(ServerItem*);

private:
	std::vector<ServerItem*> items_;
	std::vector<ServerFilterObserver*> observers_;
};

#endif
