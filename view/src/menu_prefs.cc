//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #6 $ 
//
// Copyright 2009-2016 ECMWF. 
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description : 
//=============================================================================================

#include "arch.h"
#include "menu_prefs.h"
#include "gui.h"
#include "xec.h"
#include "extent.h"
#include <ctype.h>
#include <Xm/Text.h>
#include <Xm/ToggleB.h>
#include <Xm/List.h>
#include "menus.h"

void menu_prefs::create(Widget w,char*) 
{ 
	menu_form_c::create(w); 
	prefs::setup(w); 
	build_list();
}

void menu_prefs::build_list()
{
	XmListDeleteAllItems(list_);
	menus::fillList(list_);
}

void menu_prefs::menuCB(Widget,XtPointer)
{
}

void menu_prefs::addCB(Widget,XtPointer)
{

}

void menu_prefs::removeCB(Widget,XtPointer)
{

}

void menu_prefs::check_remove()
{
}

void menu_prefs::browseCB(Widget,XtPointer data)
{
  XmListCallbackStruct *cb = (XmListCallbackStruct *) data;
  char *p = xec_GetString(cb->item);
  char *q = p;
  while(*q && *q == ' ') q++;
  XmTextSetString(title_,q);
  XtFree(p);
}


void menu_prefs::changedCB(Widget,XtPointer data)
{
}

void menu_prefs::updateCB(Widget,XtPointer data)
{
}
