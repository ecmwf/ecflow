//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #4 $ 
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

#include "depend.h"
#include "host.h"
#include "runnable.h"
#include <Xm/List.h>
#include <Xm/Text.h>
#include <Xm/ToggleB.h>
#include <Xm/PushB.h>
#include "gui.h"
#include "extent.h"
#include "flags.h"
#include "pixmap.h"
#include "result.h"
#include <X11/IntrinsicP.h>
#include <stdarg.h>

extern "C" {
#include "xec.h"
}

depend::depend()
{
	_xd_rootwidget = 0;
}

depend::~depend()
{
	if(_xd_rootwidget)
	XtDestroyWidget(_xd_rootwidget);
}

void depend::closeCB(Widget,XtPointer)  
{
	hide();
}

void depend::hide()
{
	if(_xd_rootwidget) {
		XtUnmanageChild(form_);
	}
}

void depend::make(Widget top)
{
	while(!XtIsShell(top))
		top = XtParent(top);

	if(!_xd_rootwidget)
		depend_shell_c::create(top);
}

void depend::raise(Widget top)
{
	make(top);
	XtManageChild(form_);
	XMapRaised(XtDisplay(_xd_rootwidget),XtWindow(_xd_rootwidget));
}
