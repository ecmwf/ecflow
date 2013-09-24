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

#include <stdarg.h>
#include "html_lister.h"

html_lister::html_lister(node *n):
	node_(n),
	nodes_(0), 
	cancels_(0)
{
	buf_[0] = 0;
}

html_lister::~html_lister()
{
}

void html_lister::push(node* n)
{
	char buf[1024];
	sprintf(buf,"<a href=\"%s\">%s</a>",
		n->net_name().c_str(), // TODO +1,
		n->node_name().c_str());
	strcat(buf_,buf);
	nodes_++;
}

void html_lister::push(const char* p,...) 
{ 
	char buf[1024];
	va_list arg;
	va_start(arg,p);
	vsprintf(buf,p,arg);
	va_end(arg);
	strcat(buf_,buf);
}

void html_lister::cancel()
{
	cancels_++;
}

void html_lister::endline()
{
  if(cancels_) {
		if(cancels_ >= nodes_)
		{
			//printf("Canceling line %s\n",buf_);
			buf_[0] = 0;
		}
		else
		{
			//printf("Not canceling line %s\n",buf_);
		}
  }
	line(buf_);
	buf_[0] = 0;
	nodes_ = cancels_ = 0;
}
