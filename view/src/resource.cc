//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #6 $ 
//
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description : 
//=============================================================================================

#include <strings.h>

#ifndef option_H
#include "option.h"
#endif

#ifndef base_H
#include "base.h"
#endif

#include "configurable.h"
#include "configurator.h"

resource::resource(configurable* o,const str& name,const str& value):
	owner_(o),
	base_(base::lookup(o->name())),
	name_(name),
	value_(value),
	set_(0)
{
	base_->attach();	
	base_->defaults(name_,value_);
	set_ = base_->fetch(name_,value_);
}

resource::~resource()
{
	base_->detach();
}

void resource::setset(bool on)
{
	set_ = on;

	if(on) 
		base_->store(name_,value_,true);
	else
		base_->remove(name_);

	base_->fetch(name_,value_);

	resource* r = first();
	while(r)
	{
		if(r->name() == name_)
			if(r->changed())
				r->owner_->changed(*r);
		r = r->next();
	}
}

void resource::set(const str& v)
{

	value_ = v;
	base_->store(name_,value_,true);
	set_   = true;

	resource* r = first();
	while(r)
	{
		if(r->name() == name_)
			if(r->changed())
				r->owner_->changed(*r);
		r = r->next();
	}
}

const str& resource::get()
{
	base_->fetch(name_,value_);
	return value_;
}

void resource::init(configurable& a, configurator& c)
{
	resource* r = first();
	while(r)
	{
		if(r->owner_ == &a) c.init(*r);
		r = r->next();
	}
}

void resource::modified(configurable& a, configurator& c)
{
	resource* r = first();
	while(r)
	{
		if(r->owner_ == &a) 
			if(c.modified(*r))
				a.changed(*r);
		r = r->next();
	}
}

IMP(resource)
