//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #3 $ 
//
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description : 
//=============================================================================================

#include "pref_window.h"
#include "gui.h"
#include "prefs.h"

pref_window& pref_window::instance()
{
	static pref_window *m = new pref_window();
	return *m;
}

pref_window::pref_window()
{
	create(gui::top());
	prefs::create_all(tab_);
}

pref_window::~pref_window()
{
}

void pref_window::closeCB(Widget,XtPointer)  
{
	XtUnmanageChild(form_);
}

void pref_window::mapCB(Widget,XtPointer)
{
}

void pref_window::raise()
{
	XtManageChild(form_);
	XMapRaised(XtDisplay(_xd_rootwidget),XtWindow(_xd_rootwidget));
}

void pref_window::show()
{
	instance().raise();
}
