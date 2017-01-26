//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #4 $ 
//
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description : 
//=============================================================================================

#include "option_panel.h"
#include "node.h"
#include "host.h"
#include "ecflowview.h"
#include <Xm/Label.h>
#include <Xm/Text.h>
#include <Xm/ToggleB.h>
extern "C" {
#include "xec.h"
}

option_panel::option_panel(panel_window& w):
	panel(w)
{
}

option_panel::~option_panel()
{
}

void option_panel::create (Widget parent, char *widget_name )
{
	option_form_c::create(parent,widget_name);
}

void option_panel::clear()
{
}

void option_panel::show(node& n)
{
	resource::init(n.serv(),*this);
	freeze();
}

Boolean option_panel::enabled(node& n)
{
  return n.type() == NODE_SUPER;
}

configurable* option_panel::owner()
{
  return get_node() ? &(get_node()->serv()) : 0;
}

