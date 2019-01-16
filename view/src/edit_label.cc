//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #7 $ 
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

#include "edit_label.h"
#include "label_node.h"
#include "host.h"
#include <Xm/Text.h>
extern "C" {
#include "xec.h"
}

edit_label::edit_label(panel_window& w):
	panel(w),
	loading_(false)
{
}

edit_label::~edit_label()
{
}

void edit_label::clear()
{
	loading_ = true;
	XmTextSetString(value_,"");
	XmTextSetString(default_,"");
	loading_ = false;
}

void edit_label::show(node& n)
{

	label_node& m = (label_node&)n;

	loading_ = true;
	XmTextSetString(value_,(char*)m.value());
	XmTextSetString(default_,(char*)m.def());

	loading_ = false;
}

Boolean edit_label::enabled(node& n)
{
	return n.type() == NODE_LABEL;
}

void edit_label::applyCB(Widget,XtPointer)
{
  // alter -m node value
  if(get_node())
    {
      char *p = XmTextGetString(value_);
      if (get_node()->__node__()) /* ecflow */
	get_node()->serv().command(clientName,"--alter", "change", "label",
				   get_node()->name().c_str(), p,
				   get_node()->parent_name().c_str(),
				   NULL);
      else
	get_node()->serv().command("alter", "-l",
				   get_node()->full_name().c_str(), p, NULL);
      XtFree(p);
    }
  else clear();

	submit();
}

void edit_label::changedCB(Widget,XtPointer)  
{
	if(loading_) return;
	freeze();
}

// static panel_maker<edit_label> maker(PANEL_EDIT_LABEL);
