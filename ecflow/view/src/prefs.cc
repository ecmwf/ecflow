//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #3 $ 
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

#include "prefs.h"
#include "globals.h"
#include "resource.h"

void prefs::create_all(Widget w)
{
	prefs* p = first();
	while(p)
	{
		p->create(w);
		XtManageChild(p->widget());
		p = p->next();
	}
}

void prefs::setup(Widget w)
{
	resource::init(*owner(),*this);
}

configurable* prefs::owner() 
{
	return globals::instance();
}

IMP(prefs)
