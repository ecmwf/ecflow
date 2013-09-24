//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #8 $ 
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

#include "servers_prefs.h"
#include "host.h"
#include "observer.h"
#include "gui.h"
#include "xec.h"
#include "extent.h"
#include <ctype.h>
#include <Xm/Text.h>
#include <Xm/ToggleB.h>
#include <Xm/List.h>

/* user hosts are added in map order in the 'servers' file:
   order then appears as different according to creation order and 
   alphabetic/map order
*/
static bool valid_name(const str& n)
{
  const char* p = n.c_str();
  
  if(*p == 0) return true;
  
  while(*p) {
    if(*p != '_' && !isalnum(*p)) 
      return false;
    p++;
  }
  
  return true;
}

static bool valid_host(const str& n)
{
  const char* p = n.c_str();
  
  if(*p == 0) return false;
  
  while(*p) {
    if(*p != '-' && *p != '.' && !isalnum(*p)) 
      return false;
    p++;
  }
  
  return true;
}

static bool valid_num(int)
{
  return true;
}

void servers_prefs::add_host(const std::string& h)
{
  str host = h;
  instance().add(host);
}

void servers_prefs::create(Widget w,char*) 
{ 
	servers_form_c::create(w); 
	prefs::setup(w); 
	build_list();
}

void servers_prefs::build_list()
{
  XmListDeleteAllItems(list_);
  
  array<str> x = hosts_;
  hosts_.clear();
  
  for(int i = 0; i < x.count(); i++) {
    add(x[i]);
  }
  xec_ListItemSelect(list_,current_.c_str());
}

void servers_prefs::add(str&h)
{
  hosts_.add(h);
  if(_xd_rootwidget == 0)
    return;
  
  host *x = host::find(h.c_str());
  if(x)
    xec_AddListItem(list_,(char*)h.c_str());	
}

void servers_prefs::serversCB(Widget,XtPointer)
{
  host::broadcast(true);
}

str servers_prefs::name()
{
	char *p = XmTextGetString(name_);
	char *h = XmTextGetString(host_);

	str x(*p?p:h);

	XtFree(p);
	XtFree(h);

	return x;
}

str servers_prefs::machine()
{
	char *p = XmTextGetString(name_);
	char *h = XmTextGetString(host_);

	str x(*h?h:p);

	XtFree(p);
	XtFree(h);

	return x;
}

int servers_prefs::number()
{
	
	char *n = XmTextGetString(number_);
	int x = atol(n);
	XtFree(n);

	return x?x: 3141 ;
}

void servers_prefs::addCB(Widget,XtPointer)
{
	str p = name();
	str h = machine();
	int n = number();

	host* y = host::find(p.c_str());

	int v = valid_name(p) && valid_host(h) && valid_num(n);

	if(v && (y == 0))
	{
		host::new_host(p.c_str(),h.c_str(),n);
		current_ = p;
		xec_ListItemSelect(list_,p.c_str());

		build_list(); // TBD
		XtSetSensitive(update_,false);
		XtSetSensitive(add_,false);
		XtSetSensitive(remove_,true);
		ecf_nick_write(); // TBD
	}
}

void servers_prefs::removeCB(Widget,XtPointer)
{
	xec_RemoveListItem(list_,(char*)current_.c_str());
	host::remove_host(current_.c_str());

	hosts_.remove(current_);
	current_ = "";

	XtSetSensitive(remove_,false);
	XtSetSensitive(update_,false);
	XtSetSensitive(add_,false);

	XmTextSetString(name_,"");
	XmTextSetString(host_,"");
	XmTextSetString(number_,"");

	ecf_nick_write();
}

void servers_prefs::check_remove()
{
}

void servers_prefs::browseCB(Widget,XtPointer data)
{
    XmListCallbackStruct *cb = (XmListCallbackStruct *) data;
	char *p = xec_GetString(cb->item);

	current_ = p;

	host *x = host::find(p);
	if(x)
	{
		changing_ = true;
		XmTextSetString(name_,(char*)x->name());
		XmTextSetString(host_,(char*)x->machine());

		char buf[80];
		sprintf(buf,"%d",x->number());
		XmTextSetString(number_,buf);
		changing_ = false;
	}

	XtFree(p);

	XtSetSensitive(remove_,true);
	XtSetSensitive(update_,false);
	XtSetSensitive(add_,false);
}

void servers_prefs::changedCB(Widget,XtPointer data)
{
	if(changing_) return;

	str p = name();
	str h = machine();
	int n = number();

	host* x = host::find(current_.c_str());
	host* y = host::find(p.c_str());

	int v = valid_name(p) && valid_host(h) && valid_num(n);

	if(x) {
		bool c = (current_ != p) || (h != str(x->machine())) ||
				 (n != x->number());
		XtSetSensitive(update_,v && (y == x || y == 0) && c);
	}

	XtSetSensitive(add_,v && (y == 0));
}

void servers_prefs::updateCB(Widget,XtPointer data)
{
	str p = name();
	str h = machine();
	int n = number();

	host* x = host::find(current_.c_str());
	host* y = host::find(p.c_str());

	int v = valid_name(p) && valid_host(h) && valid_num(n);
	if(v && x && (x == y || y == 0))
	{
		x->change(p.c_str(),h.c_str(),n);
		xec_ReplaceListItem(list_,current_.c_str(),p.c_str());
		xec_ListItemSelect(list_,p.c_str());
		current_ = p;
		XtSetSensitive(update_,false);
		XtSetSensitive(add_,false);
	}
}
