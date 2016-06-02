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

#include <stdio.h>
#include <errno.h>
#include <stdarg.h>
#include "error.h"
#include "gui.h"
#include "ecflowview.h"


error::error()
{
	create(gui::top());
}


error::~error()
{
}


void error::show(const char* msg)
{
	instance().modal(msg,True);
}
