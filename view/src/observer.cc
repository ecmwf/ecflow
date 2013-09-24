//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #4 $ 
//
// Copyright 2009-2012 ECMWF. 
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description : 
//=============================================================================================

#ifndef observer_H
#include "observer.h"
#endif

#ifndef relation_H
#include "relation.h"
#endif


observer::observer()
{
}

observer::~observer()
{
	relation::remove(this);
}

void observer::observe(observable* t)
{
	relation::add(this,t);
}

int observer::forget(observable* t)
{
  return relation::remove(this,t);
}

void observer::forget_all()
{
	relation::remove(this);
}

