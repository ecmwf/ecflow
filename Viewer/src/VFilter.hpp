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

	void save(boost::property_tree::ptree& array);
	void load(const boost::property_tree::ptree& array);

protected:
	void init(const std::vector<VParam*>& items);

	std::set<VParam*> all_;
	std::set<VParam*> current_;

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

#endif
