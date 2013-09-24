//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #6 $ 
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

#include "script_panel.h"
#include "node.h"
#include "host.h"
#include "ecf_node.h"
#include "variable_node.h"
#include <Xm/Text.h>
extern "C" {
#include "xec.h"
}

script_panel::script_panel(panel_window& w)
  :panel(w)
  ,text_window(false)
{
}

script_panel::~script_panel()
{
}

void script_panel::clear()
{
  XmTextSetString(name_,(char*)"");
  text_window::clear();
}

void script_panel::show(node& n)
{
  std::string p = n.variable("ECF_SCRIPT");
  if (!n.__node__()) p = n.variable("SMSSCRIPT");
  XmTextSetString(name_,p.empty() ? (char*) "" : (char*)p.c_str());
  load(n.serv().script(n));
}

Boolean script_panel::enabled(node& n)
{  
  if (n.type() != NODE_TASK && n.type() != NODE_ALIAS) return False;
  if (!n.__node__()) 
    return n.variable("SMSSCRIPT").size() > 7;
  return n.variable("ECF_SCRIPT").size() > 7;
}

void script_panel::create (Widget parent, char *widget_name )
{
	script_form_c::create(parent,widget_name);
}
