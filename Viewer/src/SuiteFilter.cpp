//============================================================================
// Copyright 2016 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "SuiteFilter.hpp"

#include "SuiteFilterObserver.hpp"
#include "UserMessage.hpp"
#include "VSettings.hpp"
#include "VProperty.hpp"

#include <algorithm>
#include <assert.h>

#define _UI_SUITEFILTER_DEBUG

std::string SuiteFilter::dummySuite_="__DUMMY_FOR_UI__";

//=================================================================
//
// SuiteFilterItem
//
//=================================================================

SuiteFilterItem::SuiteFilterItem(const SuiteFilterItem& other) :
	name_(other.name_),
    loaded_(other.loaded_),
	filtered_(other.filtered_)
{
}

//=================================================================
//
// SuiteFilter
//
//=================================================================

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
	broadcastChange();
}

void SuiteFilter::adjustLoaded(const std::vector<std::string>& loaded)
{
    bool changed=false;

    std::vector<std::string> currentLoaded;
    for(std::vector<SuiteFilterItem>::iterator it=items_.begin(); it != items_.end(); ++it)
    {
        bool ld=(std::find(loaded.begin(), loaded.end(),(*it).name_) != loaded.end());
        if((*it).loaded_ != ld)
        {
            (*it).loaded_=ld;
            if(ld &&
              loadedInitialised_ && enabled_ && autoAddNew_)
            {
                (*it).filtered_=true;
            }
            changed=true;
        }
        if(ld)
            currentLoaded.push_back((*it).name_);
    }


    //TODO: do we need to check enabled_
    for(std::vector<std::string>::const_iterator it=loaded.begin(); it != loaded.end(); ++it)
    {
        if(std::find(currentLoaded.begin(), currentLoaded.end(),*it) == currentLoaded.end())
        {
            bool filtered=loadedInitialised_ && enabled_ && autoAddNew_;
            items_.push_back(SuiteFilterItem(*it,true,filtered));
            changed=true;
        }
    }


    if(!loadedInitialised_ && loaded.size() > 0)
        loadedInitialised_=true;

    if(changed)
        broadcastChange();
}

std::vector<std::string> SuiteFilter::loaded() const
{
    std::vector<std::string> fv;
    for(std::vector<SuiteFilterItem>::const_iterator it=items_.begin(); it != items_.end(); ++it)
    {
        if((*it).loaded_)
           fv.push_back((*it).name());
    }
    return fv;
}


void SuiteFilter::adjustFiltered(const std::vector<std::string>& filtered)
{
    bool changed=false;

    std::vector<std::string> currentFiltered;
    for(std::vector<SuiteFilterItem>::iterator it=items_.begin(); it != items_.end(); ++it)
    {
        bool ld=(std::find(filtered.begin(), filtered.end(),(*it).name_) != filtered.end());
        if((*it).filtered_ != ld)
        {
            (*it).filtered_=ld;
            changed=true;
        }
        if(ld)
            currentFiltered.push_back((*it).name_);
    }

    for(std::vector<std::string>::const_iterator it=filtered.begin(); it != filtered.end(); ++it)
    {
        if(std::find(currentFiltered.begin(), currentFiltered.end(),*it) == currentFiltered.end())
        {
            items_.push_back(SuiteFilterItem(*it,false,true));
            changed=true;
        }
    }

    if(changed)
        broadcastChange();
}

std::vector<std::string> SuiteFilter::filter() const
{
    std::vector<std::string> fv;
    for(std::vector<SuiteFilterItem>::const_iterator it=items_.begin(); it != items_.end(); ++it)
    {
        if((*it).filtered_)
           fv.push_back((*it).name());
    }
    return fv;
}

void SuiteFilter::setFiltered(int index,bool val)
{
    Q_ASSERT(index >=0 && index < count());
    items_[index].filtered_=val;
}



SuiteFilter* SuiteFilter::clone()
{
    SuiteFilter* sf=new SuiteFilter();
    sf->items_=items_;
    sf->enabled_=enabled_;
    sf->autoAddNew_=autoAddNew_;

	return sf;
}

bool SuiteFilter::loadedSameAs(const std::vector<std::string>& loaded) const
{
    int cnt=0;

    for(std::vector<SuiteFilterItem>::const_iterator it=items_.begin(); it != items_.end(); ++it)
    {
        bool ld=(std::find(loaded.begin(), loaded.end(),(*it).name()) != loaded.end());
        if((*it).loaded() != ld)
           return false;
        else if(ld && (*it).loaded())
            cnt++;
    }

    return cnt == loaded.size();
}

bool SuiteFilter::setLoaded(const std::vector<std::string>& loaded,bool checkDiff)
{
    bool same=false;
    if(checkDiff)
        same=loadedSameAs(loaded);

    if(!checkDiff || !same)
    {
        adjustLoaded(loaded);
        return true;
    }

    return !same;
}

bool SuiteFilter::sameAs(const SuiteFilter* sf) const
{
    if(autoAddNew_ != sf->autoAddNewSuites())
        return false;

    if(enabled_ != sf->isEnabled())
        return false;

    if(sf->count() != count())
        return false;

    for(size_t i=0; i < items_.size(); i++)
    {
        if(items_[i] != sf->items()[i])
        {
           return false;
        }
    }

    return true;
}

bool SuiteFilter::update(SuiteFilter* sf)
{
	changeFlags_.clear();

    assert(sf);

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

    if(sf->count() != count())
    {
        items_=sf->items();
        changeFlags_.set(ItemChanged);
    }
    else
    {
        for(size_t i=0; i < items_.size(); i++)
        {
            if(items_[i] != sf->items()[i])
            {
                items_=sf->items();
                changeFlags_.set(ItemChanged);
                break;
            }
        }
    }

	return (changeFlags_.isEmpty() == false);
}

void SuiteFilter::selectAll()
{
	for(size_t i=0; i < items_.size(); i++)
	{
        items_[i].filtered_=true;        
	}
}

void SuiteFilter::unselectAll()
{
    for(size_t i=0; i < items_.size(); i++)
    {
        items_[i].filtered_=false;      
    }
}

bool SuiteFilter::removeUnloaded()
{
    std::vector<SuiteFilterItem> v;

    bool changed=false;
    for(size_t i=0; i < items_.size(); i++)
    {
        if(items_[i].loaded())
        {
            v.push_back(items_[i]);
        }
        else
        {
            changed=true;
        }
    }

    if(changed)
        items_=v;

    return changed;
}

bool SuiteFilter::hasUnloaded() const
{
    for(size_t i=0; i < items_.size(); i++)
    {
        if(!items_[i].loaded())
            return true;
    }
    return false;
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
    enabled_=vs->getAsBool("enabled",enabled_);
    autoAddNew_=vs->getAsBool("autoAddNew",autoAddNew_);

    std::vector<std::string> filter;
    if(vs->contains("suites"))
    {
        vs->get("suites",filter);
    }

    adjustFiltered(filter);

    changeFlags_.clear();
    changeFlags_.set(ItemChanged);
}

void SuiteFilter::writeSettings(VSettings *vs)
{
    vs->putAsBool("enabled",enabled_);
    vs->putAsBool("autoAddNew",autoAddNew_);

    std::vector<std::string> fv=filter();
    if(fv.size() > 0)
        vs->put("suites",fv);
}
