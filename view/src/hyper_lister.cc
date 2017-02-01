//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #6 $ 
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

#include <stdarg.h>
#include "hyper_lister.h"

const int hyper_lister::dim_ = 1024;

hyper_lister::hyper_lister(panel& o,node *n):
  owner_(o),
  node_(n),
  nodes_(0), 
  cancels_(0)
{
  buf_[0] = 0;
}

hyper_lister::~hyper_lister()
{
}

void hyper_lister::push(node* n)
{
  char buf[dim_];
  snprintf(buf,dim_,"{%s}",n->node_name().c_str());
  strcat(buf_,buf);
  nodes_++;

  owner_.observe(n);
}

void hyper_lister::push(const char* p,...) 
{ 
  char buf[dim_];
  va_list arg;
  va_start(arg,p);
  vsnprintf(buf,dim_,p,arg);
  va_end(arg);
  strcat(buf_,buf);
}

void hyper_lister::cancel()
{
  cancels_++;
}

void hyper_lister::endline()
{
  if(cancels_) {
    if(cancels_ >= nodes_) {
      //printf("Canceling line %s\n",buf_);
      buf_[0] = 0;
    }
    else {
      //printf("Not canceling line %s\n",buf_);
    }
  }
  line(buf_);
  buf_[0] = 0;
  nodes_ = cancels_ = 0;
}
