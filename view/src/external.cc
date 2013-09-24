//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #9 $ 
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

#include "external.h"
#include "host.h"
#include "ecf_node.h"

static  external* head_ = 0;

external::external(const char *name):
  node(host::dummy(),ecf_node::dummy_node())
{
  name_ = name;
  next_          = head_;
  head_          = this;
}

external::~external()
{
}

Boolean external::is_external(const char *name)
{
  return True;
}

external& external::get(const char *name)
{
  external* e = head_;
  while(e) {
    if(strcmp(name,e->name().c_str()) == 0)
      return *e;
    e = (external*)e->next_;
  }
  return *(new external(name));
}

void external::info(std::ostream&)
{
}

void external::perlify(FILE* f)
{
}

template<> 
void ecf_concrete_node<external>::set_graphic_ptr(node* n)
{}

template<> 
void ecf_concrete_node<external>::make_subtree() {}

