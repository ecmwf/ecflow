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
#include "find.h"
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

struct quick_find : public extent<quick_find> {

	str  text_;
	str  title_;
	bool regexp_;
	bool case_;

	quick_find(const str& title,const str& text,bool regexp,bool cas):
		text_(text),
		title_(title), regexp_(regexp),case_(cas) {}

	static void init(Widget);
};

void quick_find::init(Widget m)
{
  quick_find* f = quick_find::first();
  while(f)
    {
      Widget w = XmCreatePushButton(m,(char*)f->title_.c_str(),0,0);
      xec_SetUserData(w,f);
      XtManageChild(w);
      f = f->next();
    }
}

static quick_find qf1("An ECF variable","%[^%]+%",true,false);
static quick_find qf2("A shell variable",
		      "(\\$\\{[_a-z0-9]+\\})|(\\$[_a-z0-9]+)",true,false);
static quick_find qf3("A MARS error","^mars - (ERROR|FATAL)",true,true);

static quick_find qf4("ecflow_client","ecflow_client",false,true);
static quick_find qf5(" --abort"," --abort",false,true);
static quick_find qf6(" --complete"," --complete",false,true);
static quick_find qf7(" --init"," --init",false,true);
static quick_find qf8("smsabort","smsabort",false,true);

find::find():
	pending_(0)
{
	_xd_rootwidget = 0;
}

find::~find()
{
	if(_xd_rootwidget)
	XtDestroyWidget(_xd_rootwidget);
	delete pending_;
}

void find::closeCB(Widget,XtPointer)  
{
	hide();
}

void find::hide()
{
	if(_xd_rootwidget) {
		no_message();
		XtUnmanageChild(form_);
	}
}

void find::findCB(Widget,XtPointer)
{
	char* p = XmTextGetString(find_text_);
	search(p,XmToggleButtonGetState(case_),
		 XmToggleButtonGetState(regexp_),
		 XmToggleButtonGetState(back_),
		 XmToggleButtonGetState(wrap_));
	XtFree(p);
}

void find::make(Widget top)
{
	while(!XtIsShell(top))
		top = XtParent(top);

	if(!_xd_rootwidget)
	{
		find_shell_c::create(top);
		pixmap::find("QuickFind").set_label(quick_find_);
		quick_find::init(quick_menu_);
	}

}

void find::raise(Widget top)
{
	make(top);
	XtManageChild(form_);
	XMapRaised(XtDisplay(_xd_rootwidget),XtWindow(_xd_rootwidget));
}

void find::message(const char* fmt,...)
{
	if(!_xd_rootwidget)
		return;

	char buf[1024];
    va_list   args;
    va_start(args,fmt);
	vsprintf(buf,fmt,args);
	xec_SetLabel(message_,buf);
    va_end(args);

	XtManageChild(message_);
}

void find::no_message()
{
	if(_xd_rootwidget)
		XtUnmanageChild(message_);
}

void find::regexCB(Widget,XtPointer data)
{
	XmToggleButtonCallbackStruct *cb = (XmToggleButtonCallbackStruct*) data;
	if(cb->set) 
		XmToggleButtonSetState(back_,False,False);
	else
		XmToggleButtonSetState(case_,True,False);
		
	XtSetSensitive(back_,!cb->set);
	XtSetSensitive(case_, cb->set);
}

void find::entryCB(Widget,XtPointer data)
{
  XmRowColumnCallbackStruct *cb = (XmRowColumnCallbackStruct *) data;
  quick_find *w = (quick_find*) xec_GetUserData (cb->widget);
  XmTextSetString(find_text_,(char*)w->text_.c_str());
  XmToggleButtonSetState(regexp_, w->regexp_,True);
  if(w->regexp_)
    XmToggleButtonSetState(case_, w->case_  ,True);

}
IMP(quick_find)
