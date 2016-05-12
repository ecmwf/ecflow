//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #3 $ 
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

#include "edit_variable.h"
#include "host.h"
#include "node.h"
#include <Xm/Text.h>
#include <Xm/List.h>
extern "C" {
#include "xec.h"
}

edit_variable::edit_variable(panel_window& w):
	panel(w),
	loading_(false)
{
}

edit_variable::~edit_variable()
{
}

void edit_variable::clear()
{
	loading_ = true;
	xec_SetLabel(name_,"<no name>");
	XmTextSetString(value_,"");
	loading_ = false;
}

void edit_variable::show(node& n)
{
	clear();
	n.edit(*this);
	loading_ = false;
}

Boolean edit_variable::enabled(node& n)
{
  return n.type() == NODE_VARIABLE;
}

void edit_variable::applyCB(Widget,XtPointer)
{
	if(get_node())
		get_node()->apply(*this);
	else 
		clear();
	submit();
}

void edit_variable::changedCB(Widget,XtPointer)  
{
	if(loading_) return;
	freeze();
}

// static panel_maker<edit_variable> maker(PANEL_EDIT_VARIABLE);
