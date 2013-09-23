//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #3 $ 
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

#ifndef viewer_H
#include "viewer.h"
#endif

#ifndef gui_H
#include "gui.h"
#endif


viewer::viewer()
{
}

bool viewer::show(const char* cmd)
{
	FILE *f = popen(cmd,"r");
	if(!f) {
		gui::syserr(cmd);
		return false;
	}
	start(f);
	return true;
}

void viewer::ready(const char* line)
{
	gui::error("%s",line);
}

void viewer::done(FILE* f)
{
	end(pclose(f));
}

void viewer::end(bool)
{
	delete this;
}

viewer::~viewer()
{
}
