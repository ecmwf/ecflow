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

#ifndef editor_H
#include "editor.h"
#endif


Widget editor::find(const char* name)
{
	Widget w = find(name,form());
	return w;
}

Widget editor::find(const char* name,Widget p)
{
	Widget w = XtNameToWidget(p,name);
	if(w) return w;

	WidgetList wl = 0;
	int count = 0;

	XtVaGetValues(p,
		XmNchildren,&wl,
		XtNnumChildren,&count,
		NULL);

	for(int i=0; i<count; i++)
	{
		w = find(name,wl[i]);
		if(w) return w;
	}

	return 0;
}
