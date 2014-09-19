//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "VFilter.hpp"

#include "VState.hpp"
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
		all_.insert((*it)->type());
	}
}

bool VFilter::isSet(VParam::Type type) const
{
	return (current_.find(type) != current_.end());
}

void VFilter::current(const std::set<VParam::Type>& at)
{
	current_=at;
	notifyOwner();
}

void VFilter::save(boost::property_tree::ptree& array)
{
	for(std::set<VParam::Type>::const_iterator it=current_.begin(); it != current_.end(); it++)
	{
		array.push_back(std::make_pair("",toName(*it)));
	}
}

void VFilter::load(const boost::property_tree::ptree& array)
{
	current_.clear();

	for(boost::property_tree::ptree::const_iterator it = array.begin(); it != array.end(); ++it)
	{
			std::string name=it->second.get_value<std::string>();
			VParam::Type t=toType(name);
			if(all_.find(t) != all_.end())
			{
				current_.insert(t);
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
	std::vector<VParam*> v=VState::filterItems();
	init(v);
	current_=all_;
}

void StateFilter::notifyOwner()
{
	owner_->changed(this);
}

std::string StateFilter::toName(VParam::Type t)
{
	if(VParam* p=VState::find(t))
		return p->stdName();

	return std::string("");
}

VParam::Type StateFilter::toType(const std::string& name)
{
	if(VParam* p=VState::find(name))
		return p->type();

	return VParam::NoType;
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

std::string AttributeFilter::toName(VParam::Type t)
{
	if(VParam* p=VAttribute::find(t))
		return p->stdName();

	return std::string("");
}

VParam::Type AttributeFilter::toType(const std::string& name)
{
	if(VParam* p=VAttribute::find(name))
		return p->type();

	return VParam::NoType;
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

std::string IconFilter::toName(VParam::Type t)
{
	if(VParam* p=VIcon::find(t))
		return p->stdName();

	return std::string("");
}

VParam::Type IconFilter::toType(const std::string& name)
{
	if(VParam* p=VIcon::find(name))
		return p->type();

	return VParam::NoType;
}
