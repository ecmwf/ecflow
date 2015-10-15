//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "SuiteFilter.hpp"

#include "SuiteFilterObserver.hpp"
#include "VSettings.hpp"
#include "VProperty.hpp"

#include <algorithm>

//=================================================================
//
// SuiteFilterItem
//
//=================================================================

SuiteFilterItem::SuiteFilterItem(const SuiteFilterItem& other) :
	name_(other.name_),
	present_(other.present_),
	filtered_(other.filtered_)
{
}

//=================================================================
//
// SuiteFilter
//
//=================================================================

/*void SuiteFilter::current(const std::vector<std::string>& suites)
{
	current_=suites;
	adjust();
}*/

SuiteFilter::~SuiteFilter()
{
	std::vector<SuiteFilterObserver*> obsCopy=observers_;
	for(std::vector<SuiteFilterObserver*>::const_iterator it=obsCopy.begin(); it != obsCopy.end(); ++it)
	{
		(*it)->notifyDelete(this);
	}
}

void SuiteFilter::clear()
{
	items_.clear();
	filter_.clear();
	broadcastChange();
}

void SuiteFilter::adjust()
{
	items_.clear();

	//Items present in current_
	for(std::vector<std::string>::const_iterator it=loaded_.begin(); it != loaded_.end(); ++it)
	{
		bool filtered=false;
		if(std::find(filter_.begin(), filter_.end(),*it) != filter_.end())
		{
			filtered=true;
		}

		items_.push_back(SuiteFilterItem(*it,true,filtered));
	}

	//Items present in filter_ only
	for(std::vector<std::string>::const_iterator it=filter_.begin(); it != filter_.end(); ++it)
	{
		if(std::find(loaded_.begin(), loaded_.end(),*it) == loaded_.end())
		{
			items_.push_back(SuiteFilterItem(*it,false,true));
		}
	}

	broadcastChange();
}

void SuiteFilter::setFiltered(int index,bool val)
{
	if(index >=0 && index < count())
	{
		items_.at(index).filtered_=val;

		const std::string& name=items_.at(index).name_;
		std::vector<std::string>::iterator it=std::find(filter_.begin(),filter_.end(),name);

		if(val == true)
		{
			if(it == filter_.end())
				filter_.push_back(name);
		}
		else
		{
			if(it != filter_.end())
				filter_.erase(it);
		}

	}
}

SuiteFilter* SuiteFilter::clone()
{
	SuiteFilter* sf=new SuiteFilter();
	sf->loaded_=loaded_;
	sf->filter_=filter_;
	sf->items_=items_;
	sf->enabled_=enabled_;
	sf->autoAddNew_=autoAddNew_;

	return sf;
}


bool SuiteFilter::loadedSameAs(const std::vector<std::string>& loaded) const
{
	if(loaded.size() != loaded_.size())
		return false;
	else
	{
		for(unsigned int i=0; i < loaded.size(); i++)
		{
			if(loaded.at(i) != loaded_.at(i))
			{
				return false;
			}
		}
	}
	return true;
}

/*
bool SuiteFilter::setLoadedInDefs(const std::vector<std::string>& loadedInDefs)
{
	bool changed=false;
	std::vector<std::string> loadedTmp=loaded_;
	for(std::vector<std::string>::const_iterator it=loadedInDefs.begin(); it != loadedInDefs.end(); ++it)
	{
		std::vector<std::string>::const_iterator itF=std::find(loaded_.begin(),loaded_.end(),*it);
		if(itF != loaded_.end())
		{
			loadedTmp.push_back(*it);
			changed=true;
		}
	}

	if(changed)
	{
		loaded_=loadedTmp;
		adjust();
	}

	return changed;

}
*/

bool SuiteFilter::setLoaded(const std::vector<std::string>& loaded,bool checkDiff)
{
	bool same=false;
	if(checkDiff)
		same=loadedSameAs(loaded);

	if(!checkDiff || !same)
	{
		loaded_=loaded;
		adjust();
		return true;
	}

	return !same;
}


bool SuiteFilter::update(SuiteFilter* sf)
{
	changeFlags_.clear();

	if(!sf)
		return false;

	if(sf->count() != count())
		return false;

	if(autoAddNew_ != sf->autoAddNewSuites())
	{
		autoAddNew_ = sf->autoAddNewSuites();
		changeFlags_.set(AutoAddChanged);
	}

	if(enabled_ != sf->isEnabled())
	{
		enabled_ = sf->isEnabled();
		changeFlags_.set(EnabledChanged);
	}

	if(filter_.size() != sf->filter_.size())
	{
		filter_=sf->filter_;
		changeFlags_.set(ItemChanged);
	}
	else
	{
		for(size_t i=0; i < filter_.size(); i++)
		{
			if(filter_[i] != sf->filter_.at(i))
			{
				filter_[i]=sf->filter_.at(i);
				changeFlags_.set(ItemChanged);
			}
		}
	}

	adjust();

	return (changeFlags_.isEmpty() == false);
}

void SuiteFilter::selectAll()
{
	for(size_t i=0; i < items_.size(); i++)
	{
		setFiltered(static_cast<int>(i),true);
	}
}

void SuiteFilter::unselectAll()
{
	filter_.clear();
	adjust();
}

void SuiteFilter::addObserver(SuiteFilterObserver* o)
{
	assert(o);

	std::vector<SuiteFilterObserver*>::const_iterator it=std::find(observers_.begin(),observers_.end(),o);
	if(it == observers_.end())
		observers_.push_back(o);
}

void SuiteFilter::removeObserver(SuiteFilterObserver* o)
{
	std::vector<SuiteFilterObserver*>::iterator it=std::find(observers_.begin(),observers_.end(),o);
	if(it != observers_.end())
		observers_.erase(it);
}

void SuiteFilter::broadcastChange()
{
	for(std::vector<SuiteFilterObserver*>::const_iterator it=observers_.begin(); it != observers_.end(); ++it)
	{
		(*it)->notifyChange(this);
	}
}

void SuiteFilter::readSettings(VSettings *vs)
{
	clear();

	enabled_=vs->getAsBool("enabled",enabled_);
	autoAddNew_=vs->getAsBool("autoAddNew",autoAddNew_);

	if(vs->contains("suites"))
	{
		vs->get("suites",filter_);
	}

	adjust();

	changeFlags_.clear();
	changeFlags_.set(ItemChanged);
}

void SuiteFilter::writeSettings(VSettings *vs)
{
	vs->putAsBool("enabled",enabled_);
	vs->putAsBool("autoAddNew",autoAddNew_);

	if(filter_.size() >0)
	{
		std::vector<std::string> array;
		for(std::vector<std::string>::const_iterator it=filter_.begin(); it != filter_.end(); ++it)
		{
			array.push_back(*it);
		}

		vs->put("suites",array);
	}
}
