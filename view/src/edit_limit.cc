//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #10 $ 
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

#include "edit_limit.h"
#include "limit_node.h"
#include "host.h"
#include <Xm/Text.h>
#include <Xm/List.h>
extern "C" {
#include "xec.h"
}

edit_limit::edit_limit(panel_window& w):
	panel(w),
	loading_(false),
	name_(0)
{
}

edit_limit::~edit_limit()
{
	if(name_) XtFree(name_);
}

void edit_limit::clear()
{
	loading_ = true;
	XmTextSetString(max_,(char*)"");
	XmListDeleteAllItems(list_);
	XtSetSensitive(remove_,False);
	forget_all();
	loading_ = false;
	if(name_) XtFree(name_);
	name_ = 0;
}

void edit_limit::show(node& n)
{
	clear();

	limit_node& m = (limit_node&)n;

	char buf[80];
	loading_ = true;

	sprintf(buf,"%d",m.maximum()); XmTextSetString(max_,buf);

	m.nodes(*this);

	loading_ = false;
}

Boolean edit_limit::enabled(node& n)
{
	return n.type() == NODE_LIMIT;
}

void edit_limit::applyCB(Widget,XtPointer)
{
  // alter -m node value
  if(get_node()) {
    char *p = XmTextGetString(max_);
    if (1) {
    if (get_node()->__node__()) /* ecflow */
      get_node()->serv().command(clientName,"--alter", "change","limit_max",
				 get_node()->name().c_str(),p,
				 get_node()->parent_name().c_str(),
				 NULL);
    else
      get_node()->serv().command("alter", "-M", 
				 get_node()->full_name().c_str(), p, NULL);
    } else {
      std::string cmd;
      if (get_node()->__node__()) { /* ecflow */
	cmd = clientName; cmd+= "--alter change limit_max <node_name> ";
	cmd += p; cmd += " <parent_name>"; 
    } else {
	cmd = "alter -M <full_name> "; cmd += p; 
      }
      get_node()->command(cmd.c_str());
    }
    XtFree(p);
  } else {
    clear();
  }
  submit();
}

void edit_limit::changedCB(Widget,XtPointer)  
{
	if(loading_) return;
	freeze();
}

void edit_limit::browseCB(Widget,XtPointer data)
{
	XmListCallbackStruct *cb = (XmListCallbackStruct *) data;
	char *p = xec_GetString(cb->item);
	if(name_) XtFree(name_);
	name_ = p;
	XtSetSensitive(remove_,True);
}

void edit_limit::removeCB(Widget,XtPointer data)
{
  if(get_node()) {
    if(name_) {
      if (get_node()->__node__()) /* ecflow */
	get_node()->serv().command(clientName,"--alter", "delete","limit_path",
				   get_node()->name().c_str(), // limit name 
				   name_,
				   get_node()->parent_name().c_str(),
				   NULL); // task node name
      else
	get_node()->serv().command("alter", "-N",
				   get_node()->full_name().c_str(),
				   name_,
				   NULL);
    }
  } else {
    clear();
  }
}

void edit_limit::next(node& n)
{
  observe(&n);
  xec_AddListItem(list_,(char*)n.full_name().c_str());
}

void edit_limit::next(const std::string n)
{
  xec_AddListItem(list_,(char*)n.c_str());
}
