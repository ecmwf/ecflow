//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #7 $ 
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

#include "edit_meter.h"
#include "meter_node.h"
#include "host.h"
#include <Xm/Text.h>
extern "C" {
#include "xec.h"
}

edit_meter::edit_meter(panel_window& w):
	panel(w),
	loading_(false)
{
}

edit_meter::~edit_meter()
{
}

void edit_meter::clear()
{
	loading_ = true;
	XmTextSetString(min_,(char*)"");
	XmTextSetString(value_,(char*)"");
	XmTextSetString(max_,(char*)"");
	XmTextSetString(threshold_,(char*)"");
	loading_ = false;
}

void edit_meter::show(node& n)
{
	meter_node& m = (meter_node&)n;

	char buf[80];
	loading_ = true;

	sprintf(buf,"%d",m.minimum()); XmTextSetString(min_,buf);
	sprintf(buf,"%d",m.value());   XmTextSetString(value_,buf);
	sprintf(buf,"%d",m.maximum()); XmTextSetString(max_,buf);
	sprintf(buf,"%d",m.threshold()); XmTextSetString(threshold_,buf);

	loading_ = false;
}

Boolean edit_meter::enabled(node& n)
{
	return n.type() == NODE_METER;
}

void edit_meter::applyCB(Widget,XtPointer)
{
  // alter -m node value
  if(get_node())
    {
      char *p = XmTextGetString(value_);
      if (get_node()->__node__()) /* ecflow */
	get_node()->serv().command(clientName, "--alter", "change", "meter",
				   get_node()->name().c_str(), p,
				   get_node()->parent_name().c_str(), 
				   NULL);
      else
	get_node()->serv().command("alter", "-m",
				   get_node()->full_name().c_str(), p, NULL);
      XtFree(p);
    }
  else clear();

  submit();
}

void edit_meter::changedCB(Widget,XtPointer)  
{
	if(loading_) return;
	freeze();
}

// static panel_maker<edit_meter> maker(PANEL_EDIT_METER);
