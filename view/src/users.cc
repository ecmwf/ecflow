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

#include "users.h"
#include "node.h"
#include "host.h"
#include "ecflowview.h"
#include <Xm/Label.h>
#include <Xm/Text.h>
#include <Xm/List.h>
extern "C" {
#include "xec.h"
}

users::users(panel_window& w):
	panel(w)
{
}

users::~users()
{
}

void users::create (Widget parent, char *widget_name )
{
	users_form_c::create(parent,widget_name);
}

void users::clear()
{
	XmListDeleteAllItems(list_);
}

void users::show(node& n)
{
	XmListDeleteAllItems(list_);
}

Boolean users::enabled(node& n)
{
  return False;
  // return n.type() == NODE_SUPER;
}


void users::sendCB( Widget, XtPointer )
{
}

// static panel_maker<users> maker(PANEL_USERS);
