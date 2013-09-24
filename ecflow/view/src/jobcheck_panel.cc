//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #9 $ 
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

#include "jobcheck_panel.h"
#include "node.h"
#include "host.h"
#include <Xm/Text.h>
extern "C" {
#include "xec.h"
}

static const std::string cmd_str_ecf = "ECF_CHECK_CMD";
static const std::string cmd_str_sms = "SMS_CHECK_CMD";

jobcheck_panel::jobcheck_panel(panel_window& w):
	panel(w),
	text_window(false)
{
}

jobcheck_panel::~jobcheck_panel()
{
}

void jobcheck_panel::clear()
{
  text_window::clear();
}

void jobcheck_panel::show(node& n)
{
  const std::string& cmd = n.__node__() ? cmd_str_ecf : cmd_str_sms;
  const char* p = n.variable(cmd).c_str();

  if(p) 
    XmTextSetString(name_,(char*)p);
  else 
    XmTextSetString(name_,(char*)"");

  if (n.type() != NODE_TASK && n.type() != NODE_ALIAS)
    return;
  if (n.status() != STATUS_SUBMITTED &&
      n.status() != STATUS_ACTIVE    &&
      n.status() != STATUS_SUSPENDED) 
    return;
  tmp_file f = n.serv().jobcheck(n, cmd);
  text_window::load(f);
}

Boolean jobcheck_panel::enabled(node& n)
{
  if (n.type() != NODE_TASK && n.type() != NODE_ALIAS) return False;
  if (n.status() != STATUS_SUBMITTED && n.status() != STATUS_ACTIVE) return False;
  const std::string& cmd = n.__node__() ? cmd_str_ecf : cmd_str_sms;
  return n.variable(cmd).size() > 7;
}

void jobcheck_panel::create (Widget parent, char *widget_name )
{
	jobcheck_form_c::create(parent,widget_name);
}

void jobcheck_panel::update()
{
}

void jobcheck_panel::changed(node &)
{
	clear();
}

void jobcheck_panel::refresh()
{
	node* n = get_node();
	if(n) 
		show(*n);
	else 
		clear();
}

