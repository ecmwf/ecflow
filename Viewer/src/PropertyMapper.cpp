//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "PropertyMapper.hpp"

#include "UIDebug.hpp"
#include "VConfig.hpp"

PropertyMapper::PropertyMapper(const std::vector<std::string>&  names,VPropertyObserver* obs) : obs_(obs)
{
	for(std::vector<std::string>::const_iterator it=names.begin(); it != names.end(); ++it)
	{
		if(VProperty* p=VConfig::instance()->find(*it))
		{
			p->addObserver(obs);
			props_.push_back(p);
		}
	}
}

PropertyMapper::~PropertyMapper()
{
	for(std::vector<VProperty*>::const_iterator it=props_.begin(); it != props_.end(); ++it)
	{
		(*it)->removeObserver(obs_);
	}
}

VProperty* PropertyMapper::find(const std::string& path,bool failOnError) const
{
	for(std::vector<VProperty*>::const_iterator it=props_.begin(); it != props_.end(); ++it)
	{
		if((*it)->path() == path)
			return *it;
	}

    if(failOnError)
        UI_ASSERT(0,"Could not find property=" + path);

	return 0;
}

void PropertyMapper::initObserver(VPropertyObserver *obs) const
{
    for(std::vector<VProperty*>::const_iterator it=props_.begin(); it != props_.end(); ++it)
    {
        obs->notifyChange(*it);
    }
}

