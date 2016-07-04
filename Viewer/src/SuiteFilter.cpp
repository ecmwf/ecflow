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
    //filter_.clear();
	broadcastChange();
}

#if 0
void SuiteFilter::checkForNewLoaded(const std::vector<std::string>& loaded)
{
	if(enabled_ && autoAddNew_)
	{
		for(std::vector<std::string>::const_iterator it=loaded.begin(); it != loaded.end(); ++it)
		{
			if(std::find(loaded_.begin(), loaded_.end(),*it) == loaded_.end())
			{
				if(std::find(filter_.begin(), filter_.end(),*it) == filter_.end())
				{
					filter_.push_back(*it);
				}
			}
		}
	}
}
#endif

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


#if 0
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
#endif

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

#if 0
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
#endif
}



SuiteFilter* SuiteFilter::clone()
{
    SuiteFilter* sf=new SuiteFilter();
    sf->items_=items_;
    sf->enabled_=enabled_;
    sf->autoAddNew_=autoAddNew_;

#if 0
    SuiteFilter* sf=new SuiteFilter();
	sf->loaded_=loaded_;
	sf->filter_=filter_;
	sf->items_=items_;
	sf->enabled_=enabled_;
	sf->autoAddNew_=autoAddNew_;
#endif

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

#if 0
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
#endif
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

    if(!checkDiff || !same) //TODO:????
    {
        adjustLoaded(loaded);
        return true;
    }

    return !same;

#if 0
    bool same=false;
	if(checkDiff)
		same=loadedSameAs(loaded);

	if(!checkDiff || !same)
	{
        //This updates the filter
        checkForNewLoaded(loaded);
		loaded_=loaded;
        //We can have a different number of items_ if e.g. a new suite was added
        adjust();
		return true;
	}

	return !same;
#endif
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

    //if(!sf)
    //	return false;

    assert(sf);

    //if(sf->count() != count())
    //	return false;

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

#if 0
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
#endif

	return (changeFlags_.isEmpty() == false);
}

void SuiteFilter::selectAll()
{
	for(size_t i=0; i < items_.size(); i++)
	{
        items_[i].filtered_=true;
        //setFiltered(static_cast<int>(i),true);
	}
}

void SuiteFilter::unselectAll()
{
    //it cannot change loaded_ or items_
    //filter_.clear();
    for(size_t i=0; i < items_.size(); i++)
    {
        items_[i].filtered_=false;
        //setFiltered(static_cast<int>(i),false);
    }

    //adjust();
}

bool SuiteFilter::removeUnloaded()
{
    //it cannot change loaded_ or items_
    //filter_.clear();
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

#if 0
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
#endif

}

void SuiteFilter::writeSettings(VSettings *vs)
{
    vs->putAsBool("enabled",enabled_);
    vs->putAsBool("autoAddNew",autoAddNew_);

    std::vector<std::string> fv=filter();
    if(fv.size() > 0)
        vs->put("suites",fv);

#if 0
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
#endif
}
