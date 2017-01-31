//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #8 $ 
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

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <Xm/Xm.h>
// #include <ostream.h>

#include "base.h"
#include "directory.h"
#include "str.h"

struct pairs {

	pairs* next_;
	str  name_;
	str  value_;

	pairs(const str& n,const str& v, pairs* x): 
	   next_(x), name_(n),value_(v) {}

	~pairs() { delete next_; }
};


static base* defbase = 0;

base::base(const str& name,const str& dir,bool save,base* p):
	name_(name),
	dir_(dir),
	count_(0),
	pairs_(0),
	parent_(p),
	save_(save)
{
}

base::~base()
{
}

base* base::lookup(const str& name)
{
  if(defbase == 0) {
    defbase = new base("user.default",directory::user(),true,
		       new base("system.default",directory::system(),true, 
				new base(str(),str(),false,0)
				)
		       );
  }

  base *p = extent<base>::first();
  while(p) {
    if(p->name_ == name)
      return p;
    p = p->extent<base>::next();
  }

  return new base(name,directory::user(),true,defbase);
}

void base::attach()
{
  if(parent_) 
    parent_->attach();

  count_++;

  if(count_ == 1 && save_) {
  // if(save_) {
    char buf[1024];
    sprintf(buf,"%s/%s.options",dir_.c_str(),name_.c_str());
    
    FILE* f = fopen(buf,"r");
    if(f) {
      while(fgets(buf,sizeof(buf),f)) {
	char *p = buf;
	while(*p && *p != ':') p++;
	if(*p != ':') continue;
	
	buf[strlen(buf)-1] = 0;
	*p = 0;
	
	store(buf,p+1,true);
      }
      fclose(f);
    }
  }
}

void base::detach()
{
  if(parent_) 
    parent_->detach();

  count_--;
  
  if(count_ == 0 && save_)
    save();
}

void base::save()
{
  char buf[1024];
  sprintf(buf,"%s/%s.options",dir_.c_str(),name_.c_str());
  // fprintf(stdout, "%s\n", buf);
  
  FILE* f = fopen(buf,"w");
  if(f) {
    pairs* p = pairs_;
    while(p) {
      fprintf(f,"%s:%s\n",p->name_.c_str(),p->value_.c_str());	
      p = p->next_;
    }
    fclose(f);
  }
}

bool base::fetch(const str& key,str& value)
{
  pairs* p = pairs_;
  while(p) {
    if(p->name_ == key) {
      value = p->value_;
      return true;
    }
    p = p->next_;
  }
  
  if(parent_) 
    parent_->fetch(key,value);
  return false;
}

void base::defaults(const str& key,const str& value)
{
  if(parent_) {
    parent_->defaults(key,value);
  } else {
    store(key,value,false);
  }
}

void base::remove(const str& key)
{
  pairs* p = pairs_;
  pairs* q = 0;
  
  while(p) {
    if(p->name_ == key) {
      if(q) q->next_ = p->next_;
      else pairs_ = p->next_;
      p->next_ = 0;
      
      delete p;
      
      remove(key);
      return;
    }
    q = p;
    p = p->next_;
  }
  enable();
}

void base::store(const str& key,const str& value,bool replace)
{
  pairs* p = pairs_;
  enable();
  while(p) {
    if(p->name_ == key) {
      if(replace) {
	p->value_ = value;
      }
      return;
    }
    p = p->next_;
  }

  pairs_ = new pairs(key,value,pairs_);
}

void base::run()
{
  if(save_) 
    save();
  disable();
}

IMP(base)
