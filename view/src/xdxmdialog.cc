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

#include <Xm/DialogS.h>

#include <xdclass.h>

xd_XmDialog_c::xd_XmDialog_c()
{
    xd_childwidget=0;
    return;
}

Widget xd_XmDialog_c::xd_getchildwidget()
{
    if (!xd_childwidget)
    {
	Cardinal numchildren;
	GetValue(XmNnumChildren, &numchildren);
	if (numchildren!=0)
	{
	    WidgetList children;
	    GetValue(XmNchildren, &children);
	    xd_childwidget=*children;
	}
    }
    return xd_childwidget;
}

void xd_XmDialog_c::xd_show()
{
    if (!_xd_rootwidget) // Nothing to show
	return;
    if (!xd_getchildwidget()) // Nothing to show
	return;
    XtManageChild(xd_childwidget);
    return;
}

void xd_XmDialog_c::xd_hide()
{
    if (!_xd_rootwidget) // Nothing to hide
	return;
    if (!xd_getchildwidget()) // Nothing to hide
	return;
    XtUnmanageChild(xd_childwidget);
    return;
}

void xd_XmDialog_c::Raise()
{
    (void) XRaiseWindow(XtDisplay(_xd_rootwidget), XtWindow(_xd_rootwidget));
    return;
}
