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
#include <cassert>

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
    for(auto & item : items_)
    {
        bool ld=(std::find(loaded.begin(), loaded.end(),item.name_) != loaded.end());
        if(item.loaded_ != ld)
        {
            item.loaded_=ld;
            if(ld &&
              loadedInitialised_ && enabled_ && autoAddNew_)
            {
                if(!item.filtered_)
                {
                    filteredChanged=true;
                }

                item.filtered_=true;
            }
            changed=true;
        }
        if(ld)
            currentLoaded.push_back(item.name_);
    }


    //TODO: do we need to check enabled_
    for(const auto & it : loaded)
    {
        if(std::find(currentLoaded.begin(), currentLoaded.end(),it) == currentLoaded.end())
        {
            bool filtered=loadedInitialised_ && enabled_ && autoAddNew_;
            items_.push_back(SuiteFilterItem(it,true,filtered));
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
    for(const auto & item : items_)
    {
        if(item.loaded_)
           fv.push_back(item.name());
    }
    return fv;
}

//Called only once on init
void SuiteFilter::adjustFiltered(const std::vector<std::string>& filtered)
{
    bool changed=false;

    std::vector<std::string> currentFiltered;
    for(auto & item : items_)
    {
        bool ld=(std::find(filtered.begin(), filtered.end(),item.name_) != filtered.end());
        if(item.filtered_ != ld)
        {
            item.filtered_=ld;
            changed=true;                      
        }
        if(ld)
            currentFiltered.push_back(item.name_);
    }

    for(const auto & it : filtered)
    {
        if(std::find(currentFiltered.begin(), currentFiltered.end(),it) == currentFiltered.end())
        {
            items_.push_back(SuiteFilterItem(it,false,true));
            changed=true;
        }
    }

    if(changed)
        broadcastChange();
}

std::vector<std::string> SuiteFilter::filter() const
{
    std::vector<std::string> fv;
    for(const auto & item : items_)
    {
        if(item.filtered_)
           fv.push_back(item.name());
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
    for(auto & item : items_)
    {
        if(item.name() == oneSuite)
        {
            item.filtered_=true;
            return;
        }
    }
}

SuiteFilter* SuiteFilter::clone()
{
    auto* sf=new SuiteFilter();
    sf->items_=items_;
    sf->enabled_=enabled_;
    sf->autoAddNew_=autoAddNew_;

	return sf;
}

bool SuiteFilter::loadedSameAs(const std::vector<std::string>& loaded) const
{
    int cnt=0;

    for(const auto & item : items_)
    {
        bool ld=(std::find(loaded.begin(), loaded.end(),item.name()) != loaded.end());
        if(item.loaded() != ld)
           return false;
        else if(ld && item.loaded())
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
    for(auto & oriItem : oriItems)
    {
        for(auto & item : items_)
        {
            if(item.name_ == oriItem.name_)
            {
                if(item.filtered_ != oriItem.filtered_ )
                {
                    filteredChanged=true;
                }

                //we keep the filtered state from the original items
                item.filtered_ = oriItem.filtered_;

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
	for(auto & item : items_)
	{
        item.filtered_=true;        
	}
}

void SuiteFilter::unselectAll()
{
    for(auto & item : items_)
    {
        item.filtered_=false;      
    }
}

bool SuiteFilter::removeUnloaded()
{
    std::vector<SuiteFilterItem> v;

    bool changed=false;
    for(auto & item : items_)
    {
        if(item.loaded())
        {
            v.push_back(item);
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
    for(const auto & item : items_)
    {
        if(!item.loaded())
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
	auto it=std::find(observers_.begin(),observers_.end(),o);
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
    for(const auto & item : obj.items_)
    {
        aStream << " " << item.name() << " " << item.filtered() << " " << item.loaded() << std::endl;
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
