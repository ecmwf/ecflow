//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #6 $ 
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

#include "manual.h"
#include "host.h"
#include "node.h"
#include <Xm/Text.h>

extern "C" {
#include "xec.h"
}

manual::manual(panel_window& w):
	panel(w),
	text_window(false)
{
}

manual::~manual()
{
}

void manual::clear()
{
	text_window::clear();
}

void manual::show(node& n)
{
	load(n.serv().manual(n));
}

Boolean manual::enabled(node& n)
{
	return n.hasManual();
}

void manual::create (Widget parent, char *widget_name )
{
	manual_form_c::create(parent,widget_name);
}
