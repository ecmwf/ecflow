//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef VIEWFILTER_HPP_
#define VIEWFILTER_HPP_

#include <set>

#include "VConfig.hpp"
#include "VParam.hpp"

#include "Node.hpp"

class ServerHandler;
class VSettings;

#include <boost/property_tree/ptree.hpp>

class VFilter : public VConfigItem
{
public:
	VFilter(VConfig* owner);
	virtual ~VFilter() {};

	const std::set<VParam*>& current() const {return current_;}
	void current(const std::set<std::string>&);

	bool isEmpty() const {return current_.size() ==0 ;}
	bool isComplete() const { return all_.size() == current_.size();}
	bool isSet(const std::string&) const;
	bool isSet(VParam*) const;

	void writeSettings(VSettings* vs);
	void readSettings(VSettings* vs);


protected:
	void init(const std::vector<VParam*>& items);

	std::set<VParam*> all_;
	std::set<VParam*> current_;
	std::string settingsId_;

};

class StateFilter : public VFilter
{
public:
	StateFilter(VConfig* owner);
	void notifyOwner();
};

class AttributeFilter : public VFilter
{
public:
	AttributeFilter(VConfig* owner);
	void notifyOwner();
};

class IconFilter : public VFilter
{
public:
	IconFilter(VConfig* owner);
	void notifyOwner();
};


class NodeFilter
{
public:
	NodeFilter();
	virtual ~NodeFilter() {};

	virtual void reset(ServerHandler* server,VFilter* sf)=0;
    virtual bool isFiltered(Node* node)=0;
    virtual int  matchCount()=0;
    virtual int  nonMatchCount()=0;
    virtual Node* match(int i)=0;

protected:
	std::set<std::string> type_;
	std::set<std::string> state_;
};

class TreeNodeFilter : public NodeFilter
{
public:
	TreeNodeFilter();
	void reset(ServerHandler* server,VFilter* sf);
	bool isFiltered(Node* node);
	int  matchCount() {return -1;};
	int  nonMatchCount() {return static_cast<int>(nonMatch_.size());};
	Node* match(int i) {return NULL;}

private:
	bool filterState(node_ptr node,VFilter* stateFilter);
	std::set<Node*> nonMatch_;
};

class TableNodeFilter : public NodeFilter
{
public:
	TableNodeFilter();
	void reset(ServerHandler* server,VFilter* sf);
	bool isFiltered(Node* node);
	int  matchCount() {return static_cast<int>(match_.size());};
	int  nonMatchCount() {return -1;}
	Node* match(int i);

private:
	std::vector<Node*> match_;
};

#endif
