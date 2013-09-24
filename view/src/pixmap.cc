//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #5 $ 
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

#include <X11/xpm.h>
#include <ctype.h>
#include "pixmap.h"
#include "gui.h"
#include "directory.h"

pixmap::pixmap(const char* name):
	pixmap_(0),
	name_(clean(name)),
	bits_(0)
{
}

pixmap::pixmap(const char* name,const char** bits):
	pixmap_(0),
	name_(clean(name)),
	bits_(bits)
{
}


pixmap::~pixmap()
{
}

pixmap& pixmap::find(const char* name)
{
	pixmap *r = first();
	const char* n = clean(name);
	while(r)
	{
		if(strcmp(n,r->name_.c_str()) == 0)
			return *r;
		r = r->next();
	}
	return *(new pixmap(name));
}

void pixmap::set_label(Widget w)
{
	XtVaSetValues(w,
		XmNlabelType, XmPIXMAP,
		XmNlabelPixmap,pixels(),
		NULL);
}

Pixmap pixmap::pixels()
{
	if(pixmap_) return pixmap_;

	XpmAttributes xpm_attributes;
	xpm_attributes.valuemask = XpmCloseness;
	xpm_attributes.closeness = 65535;

	Display* dpy = XtDisplay(gui::top());

	if(bits_)
	{
		if(XpmCreatePixmapFromData(dpy,
			DefaultRootWindow(dpy),
			(char**)bits_,&pixmap_,0,&xpm_attributes))
				pixmap_ = XmUNSPECIFIED_PIXMAP;
	}
	else {
	  char buf[1024];
	  sprintf(buf,"%s/icons/%s.xpm",directory::system(),name_.c_str());
	  
	  if(XpmReadFileToPixmap(dpy,
				 DefaultRootWindow(dpy),
				 buf, &pixmap_, 0, &xpm_attributes))
	    {
	      pixmap_ =  XmUNSPECIFIED_PIXMAP;
	    }
	}
	
	return pixmap_;       
}

const char* pixmap::clean(const char* s)
{
	static char n[1024];
	strcpy(n,s);
	char *p = n;
	while(*p) { if(!isalnum(*p)) *p = '_'; p++; }
	return n;
}

IMP(pixmap)
