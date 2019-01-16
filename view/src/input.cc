//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #3 $ 
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

#include <stdio.h>
#include <strings.h>
#include "ecflowview.h"
#include "input.h"

extern XtAppContext app_context;

void input::inputCB(XtPointer data,int*,XtInputId* id)
{
	input* p = ((input*)data);
	char buf[1024];

	if(fgets(buf,sizeof(buf),p->file_))
	{
		if(buf[0]) buf[strlen(buf)-1] = 0;
		p->ready(buf);
	}
	else 
		p->done(p->file_);
}

input::input():
	id_(0),
	file_(0)
{
}

input::~input()
{
	stop();
}

void input::stop()
{
	if(file_)
	{
		XtRemoveInput(id_);
		file_ = 0;
	}
}

void input::start(FILE* f)
{
	if(file_ == 0)
	{
		file_ = f;
		id_ = XtAppAddInput(app_context,fileno(f),
			XtPointer(XtInputReadMask),
			inputCB,this);
	}
}

