//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "VFilter.hpp"

#include "VNState.hpp"
#include "VAttribute.hpp"
#include "VIcon.hpp"
#include "VParam.hpp"

//==============================================
//
// VFilter
//
//==============================================

VFilter::VFilter(VConfig * owner) : VConfigItem(owner)
{
}

void VFilter::init(const std::vector<VParam*>& items)
{
	for(std::vector<VParam*>::const_iterator it=items.begin(); it != items.end(); it++)
	{
		all_.insert((*it));
	}
}

bool VFilter::isSet(VParam* p) const
{
	return (current_.find(p) != current_.end());
}

bool VFilter::isSet(const std::string &name) const
{
	for(std::set<VParam*>::const_iterator it=current_.begin(); it != current_.end(); it++)
	{
		if((*it)->name() == name)
				return true;
	}
	return false;
}

void VFilter::current(const std::set<std::string>& names)
{
	current_.clear();
	for(std::set<VParam*>::const_iterator it=all_.begin(); it != all_.end(); it++)
	{
			if(names.find((*it)->name()) != names.end())
				current_.insert(*it);
	}
	notifyOwner();
}

void VFilter::save(boost::property_tree::ptree& array)
{
	for(std::set<VParam*>::const_iterator it=current_.begin(); it != current_.end(); it++)
	{
		array.push_back(std::make_pair("",(*it)->name()));
	}
}

void VFilter::load(const boost::property_tree::ptree& array)
{
	current_.clear();

	for(boost::property_tree::ptree::const_iterator it = array.begin(); it != array.end(); ++it)
	{
			std::string name=it->second.get_value<std::string>();
			for(std::set<VParam*>::const_iterator it=all_.begin(); it != all_.end(); it++)
			{
					if((*it)->name() == name)
						current_.insert(*it);
			}
	}
}

//==============================================
//
// StateFilter
//
//==============================================

StateFilter::StateFilter(VConfig * owner) : VFilter(owner)
{
	std::vector<VParam*> v=VNState::filterItems();
	init(v);
	current_=all_;
}

void StateFilter::notifyOwner()
{
	owner_->changed(this);
}

//==============================================
//
// AttributeFilter
//
//==============================================

AttributeFilter::AttributeFilter(VConfig * owner) : VFilter(owner)
{
	std::vector<VParam*> v=VAttribute::filterItems();
	init(v);
}

void AttributeFilter::notifyOwner()
{
	owner_->changed(this);
}

//==============================================
//
// IconFilter
//
//==============================================

IconFilter::IconFilter(VConfig * owner) : VFilter(owner)
{
	std::vector<VParam*> v=VIcon::filterItems();
	init(v);
	current_=all_;
}

void IconFilter::notifyOwner()
{
	owner_->changed(this);
}
