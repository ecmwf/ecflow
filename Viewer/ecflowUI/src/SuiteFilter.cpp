//============================================================================
// Copyright 2009-2017 ECMWF.
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
#include "UiLog.hpp"

#include <algorithm>
#include <assert.h>

#define SUITEFILTER_UI_DEBUG_

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

bool SuiteFilter::adjustLoaded(const std::vector<std::string>& loaded)
{
#ifdef SUITEFILTER_UI_DEBUG_
    UI_FUNCTION_LOG
#endif
    bool changed=false;
    bool filteredChanged=false;

#ifdef SUITEFILTER_UI_DEBUG_
    UiLog().dbg() << "loaded=" << loaded;
    UiLog().dbg() << "items_=" << *this;
#endif

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
                if(!(*it).filtered_)
                {
                    filteredChanged=true;
                }

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
            if(filtered)
            {
                filteredChanged=true;
            }
        }
    }

    if(!loadedInitialised_ && loaded.size() > 0)
        loadedInitialised_=true;

    if(changed)
        broadcastChange();

#ifdef SUITEFILTER_UI_DEBUG_
    UiLog().dbg() << "(2) items_=" << *this;
#endif

    return filteredChanged;
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

//Called only once on init
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

#ifdef SUITEFILTER_UI_DEBUG_
    UI_FUNCTION_LOG
    UiLog().dbg() << "filter=" << fv;
#endif

    return fv;
}

void SuiteFilter::setFiltered(int index,bool val)
{
    Q_ASSERT(index >=0 && index < count());
    items_[index].filtered_=val;
}


bool SuiteFilter::isOnlyOneFiltered(const std::string& oneSuite) const
{
    const std::vector<std::string>& current=filter();
    return (current.size() == 1 && current[0] != oneSuite);
}

void SuiteFilter::selectOnlyOne(const std::string& oneSuite)
{
    if(isOnlyOneFiltered(oneSuite))
        return;

    unselectAll();
    for(std::vector<SuiteFilterItem>::iterator it=items_.begin(); it != items_.end(); ++it)
    {
        if((*it).name() == oneSuite)
        {
            (*it).filtered_=true;
            return;
        }
    }
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

    return cnt == static_cast<int>(loaded.size());
}

bool SuiteFilter::setLoaded(const std::vector<std::string>& loaded,bool checkDiff)
{
    bool same=false;
    if(checkDiff)
        same=loadedSameAs(loaded);

    if(!checkDiff || !same)
    {
        return adjustLoaded(loaded);
    }

    return false;
    //return !same;
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

bool SuiteFilter::sameAsLoadedIgnored(const SuiteFilter* sf) const
{
    if(autoAddNew_ != sf->autoAddNewSuites())
        return false;

    if(enabled_ != sf->isEnabled())
        return false;

    if(sf->count() != count())
        return false;

    for(size_t i=0; i < items_.size(); i++)
    {
        if(items_[i].filtered_ != sf->items()[i].filtered_ ||
           items_[i].name_ != sf->items()[i].name_ )
        {
           return false;
        }
    }

    return true;
}

bool SuiteFilter::merge(const SuiteFilter* sf)
{
    bool filteredChanged=false;

    if(enabled_ != sf->enabled_)
    {
        filteredChanged=true;
    }
    //enabled_=sf->enabled_;

    if(autoAddNew_ !=sf->autoAddNew_ )
    {
       filteredChanged=true;
    }
    //autoAddNew_=sf->autoAddNew_;

    std::vector<SuiteFilterItem> oriItems=items_;

    items_=sf->items_;
    for(size_t i=0; i < oriItems.size(); i++)
    {
        for(size_t j=0; j < items_.size(); j++)
        {
            if(items_[j].name_ == oriItems[i].name_)
            {
                if(items_[j].filtered_ != oriItems[i].filtered_ )
                {
                    filteredChanged=true;
                }

                //we keep the filtered state from the original items
                items_[j].filtered_ = oriItems[i].filtered_;

                break;
            }
        }
    }

    return filteredChanged;
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

    bool changed=(changeFlags_.isEmpty() == false);
    if(changed)
    {
        broadcastChange();
    }
    return changed;
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

std::ostream& operator<< ( std::ostream& aStream, const SuiteFilter& obj)
{
    aStream << " " << obj.loadedInitialised_ << " " << obj.enabled_  << " " << obj.autoAddNew_ << std::endl;
    for(std::vector<SuiteFilterItem>::const_iterator it=obj.items_.begin(); it != obj.items_.end(); ++it)
    {
        aStream << " " << (*it).name() << " " << (*it).filtered() << " " << (*it).loaded() << std::endl;
    }
    return aStream;
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
