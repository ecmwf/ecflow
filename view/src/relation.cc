//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #4 $ 
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

#include "runnable.h"
#include "relation.h"
#include "observable.h"
#include "counted.h"
#include <stdio.h>

class run_gc : public runnable {
	void run()  { if(!relation::gc()) disable();}
};


static run_gc dogc;

relation::relation(observer* a,observable* b):
  observer_(a),
  observable_(b),
  data_(0),
  valid_(true)
{
}

relation::~relation()
{
  if(data_) 
    data_->detach();
}

void relation::add(observer* a,observable* b)
{
  if(a && b) {
    b->observed_ = true;
    new relation(a,b);
  }
}

void relation::stats(const char* p)
{  
  relation* r = first();
  int c = 0; int v = 0;
  while(r) {
    relation* n = r->next();
    c++;
    if(r->valid_) v++;
    r = n;
  }
  
  // printf("relation::stat %s %d relation(s) %d valid(s)\n",p,c,v);
}

int relation::remove(observer* a,observable* b)
{	
  int rc = 0;
  relation* r = first();
  while(r) {
    relation* n = r->next();
    if(r->observer_ == a && r->observable_ == b) {
      r->valid_ = false;
      rc++;
    }
    r = n;
  }

  dogc.enable();
  return rc;
}

int relation::remove(observer* a)
{
  int rc = 0;
  relation* r = first();
  while(r) {
    relation* n = r->next();
    if(r->observer_ == a) {
      r->valid_ = false;
      rc++;
    }
    r = n;
  }

  dogc.enable();  
  return rc;
}

int relation::remove(observable* b)
{
  int rc = 0;
  relation* r = first();
  while(r) {
    relation* n = r->next();
    if(r->observable_ == b) {
      r->valid_ = false;
      rc++;
    }
    r = n;
  }

  dogc.enable();
  return rc;
}

void relation::replace(observable* o,observable* x)
{
  relation* r = first();
  while(r) {
    relation* n = r->next();
    if(r->observable_ == o)
      r->observable_ = x;
    r = n;
  }
}

void relation::scan(observable* b,observer_iterator& i)
{
  relation* r = first();
  
  while(r) {
    relation* n = r->next();
    if(r->observable_ == b && r->valid_)
      i.next(r->observer_);
    r = n;
  }
}

bool relation::gc()
{
  relation* r = first();
  
  stats("relation::gc"); 
  
  while(r) {
    relation* n = r->next();
    if(!r->valid_) {
      delete r;
      return true;
    }
    r = n;
  }
  
  return false;
}

void relation::set_data(observer* a,observable* b,counted* x)
{
  relation* r = first();
  while(r) {
    relation* n = r->next();
    if(r->observer_ == a && r->observable_ == b) {
      if(x != r->data_) {
	if(r->data_) r->data_->detach();
	r->data_ = x;
	if(r->data_) r->data_->attach();
      }
      return;
    }
    r = n;
  }
}

counted* relation::get_data(observer* a,observable* b)
{
  relation* r = first();
  while(r) {
    relation* n = r->next();
    if(r->observer_ == a && r->observable_ == b)
      return r->data_;
    r = n;
  }
  return 0;
}

IMP(relation)
