// Copyright 2015 ECMWF.

#include "PropertyMapper.hpp"

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


VProperty* PropertyMapper::find(const std::string& path) const
{
	for(std::vector<VProperty*>::const_iterator it=props_.begin(); it != props_.end(); ++it)
	{
		if((*it)->path() == path)
			return *it;
	}

	return 0;
}
