//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #3 $ 
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

#include "pref_editor.h"
#include "node.h"
#include "host.h"
#include "ecflowview.h"
#include "str.h"
#include <Xm/Label.h>
#include <Xm/Text.h>
#include <Xm/ToggleB.h>
extern "C" {
#include "xec.h"
}


void pref_editor::init(resource& x)
{
	Widget w = find(x.name().c_str()); if(!w) return;
	x.initWidget(w);
	sensitive(w,x.name().c_str(),x.isSet());
}

void pref_editor::sensitive(Widget w, const char* n, bool set)
{
	Widget p = toggle(n); if(!p) return;
	XtSetSensitive(w,set);
	XmToggleButtonSetState(p,!set,False);
}

Widget pref_editor::toggle(const char* n)
{
	char buf[1024];
	sprintf(buf,"@%s",n);
	return find(buf); 
}

bool pref_editor::modified(resource& x)
{
	Widget w     = find(x.name().c_str());   if(!w) return false;
	bool on      = XtIsSensitive(w);

	if(on != x.isSet())
	{
		x.setset(on);
		if(on)  x.readWidget(w);
		return true;
	}


	return on ? x.readWidget(w) : false;
}

void pref_editor::changed(Widget w)
{
	configurable* c = owner();
	if(c) 
	{
		resource::modified(*c,*this);
		resource::init(*c,*this);
	}
}

void pref_editor::use(Widget w)
{
	bool s  = XmToggleButtonGetState(w);
	char* p = XtName(w);

	
	Widget z = find(p+1);
	if(z) XtSetSensitive(z,!s);

	changed(w);
}

