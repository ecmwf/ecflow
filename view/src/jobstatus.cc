
//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #13 $ 
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

#include "jobstatus.h"
#include "node.h"
#include "host.h"
#include <Xm/Text.h>
#include "ecf_node.h"
extern "C" {
#include "xec.h"
}

static const std::string cmd_str_ecf = "ECF_STATUS_CMD";
static const std::string cmd_str_sms = "SMSSTATUSCMD";

jobstatus::jobstatus(panel_window& w):
  panel(w),  
  text_window(false)
  , reload_(true)
{
}

jobstatus::~jobstatus()
{
}

void jobstatus::clear()
{
  XmTextSetString(name_,(char*)"");
  text_window::clear();
}

void jobstatus::show(node& n)
{
  ecf_node* ecf = n.__node__();
  const std::string& scmd = ecf ? cmd_str_ecf : cmd_str_sms;
  const std::string var = n.variable(scmd, true);
  const std::string& job = ecf ? n.variable("ECF_JOB") : n.variable("SMSJOB");
  std::string stat   = job + ".stat";

  if (!var.empty())
    XmTextSetString(name_,(char*)var.c_str());
  else if(!scmd.empty()) 
    XmTextSetString(name_,(char*)scmd.c_str());
  else {
    std::string cmd = scmd + "%s variable does not exist";
    XmTextSetString(name_,(char*)cmd.c_str());
  }
  if (n.type() != NODE_TASK && n.type() != NODE_ALIAS) {
    XmTextSetString(name_,(char*)"not a task");
    return;
  } 
  if (n.status() != STATUS_SUBMITTED &&
      n.status() != STATUS_ACTIVE    &&
      n.status() != STATUS_SUSPENDED) {
    XmTextSetString(name_,(char*)"not submitted not active");
    return;
  }
  if (reload_) {  
    reload_ = false;
    tmp_file (n.serv().jobstatus(n, "")); 
  }
  tmp_file f (stat.c_str(), false);
  text_window::load(f);
}

void jobstatus::updateCB(Widget,XtPointer data)
{
  reload_ = true;
  if(get_node())
    show(*get_node());
  else
    clear();
  XmTextShowPosition(text_,XmTextGetLastPosition(text_));
}

Boolean jobstatus::enabled(node& n)
{

  if (n.type() != NODE_TASK && n.type() != NODE_ALIAS)
    return False;

  if (n.status() != STATUS_SUBMITTED && 
      n.status() != STATUS_ACTIVE    &&
      n.status() != STATUS_SUSPENDED)
    return False;

  const std::string& cmd = n.__node__() ? cmd_str_ecf : cmd_str_sms;
  return n.variable(cmd).size() > 6; 
}

void jobstatus::create (Widget parent, char *widget_name )
{
	jobstatus_form_c::create(parent,widget_name);
}
