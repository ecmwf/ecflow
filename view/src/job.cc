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

#include "job.h"
#include "node.h"
#include "host.h"
#include "ecf_node.h"
#include <Xm/Text.h>
extern "C" {
#include "xec.h"
}

job::job(panel_window& w):
	panel(w),
	text_window(false)
{
}

job::~job()
{
}

void job::clear()
{
	XmTextSetString(name_,"");
	text_window::clear();
}

void job::show(node& n)
{
  const std::string& job = n.__node__() ? 
    n.variable("ECF_JOB") : n.variable("SMSJOB");
  XmTextSetString(name_, (char*) job.c_str());
  load(n.serv().job(n));
}

Boolean job::enabled(node& n)
{
  if (n.type() != NODE_TASK && n.type() != NODE_ALIAS) return False;  
  const std::string& job = n.__node__() ? 
    n.variable("ECF_JOB") : n.variable("SMSJOB");
  return job.size() > 7;
}

void job::create (Widget parent, char *widget_name )
{
	job_form_c::create(parent,widget_name);
}

