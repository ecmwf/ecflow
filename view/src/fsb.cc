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

#include <stdarg.h>
#include <stdio.h>
#include "fsb.h"
#include "gui.h"
extern "C" {
#include "xec.h"
}

fsb::fsb():
	file_(0)
{
	create(gui::top());
}

fsb::~fsb()
{
	if(file_) XtFree(file_);
}


const char* fsb::ask(const char* title,
	const char* file,const char* filter,
	const char* dir)
{
	return instance().choose(title,file,filter,dir);
}


const char* fsb::choose(const char* title,
	const char* deffile,const char* filter,
	const char* directory)

{
	if(file_) XtFree(file_);
	file_ = 0;

	set(XmNdirSpec,deffile);
	set(XmNpattern,filter);
	set(XmNdirectory,directory);

	instance().modal(title,True);

	return file_;

}

void fsb::set(const char* res,const char* val)
{
	if(!val) return;
	XmString s = XmStringCreateSimple((char*)val);
	XtVaSetValues(form_,res,s,NULL);
	XmStringFree(s);
}

void fsb::okCB(Widget,XtPointer data)
{
	XmFileSelectionBoxCallbackStruct* cb = (XmFileSelectionBoxCallbackStruct*)data;

	char buf[1024];

	char *f = (char*)xec_GetString(cb->value);
	char *d = (char*)xec_GetString(cb->dir);

	if(*f == '/')
		strcpy(buf,f);
	else    
		sprintf(buf,"%s%s",d,f);

	XtFree(f);
	XtFree(d);

	file_ = XtNewString(buf);

	ok_   = true;
	stop_ = true;
}
