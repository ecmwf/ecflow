//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #15 $ 
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

#include "dummy_node.h"
#include "host.h"

#ifndef ecf_node_
#include "ecf_node.h"
#endif
#include <boost/shared_ptr.hpp>

static node* head_ = 0;
static const std::string id = "(dummy_node)";
const std::string dummy_node::toString() const { return id; }

dummy_node::dummy_node(const std::string name)
  : node(host::dummy(),0)
  , name_ (name)
{
  next_          = head_;
  head_          = this;
  owner_ = boost::shared_ptr<ecf_node>(new ecf_concrete_node<dummy_node> (this, 0));
}

dummy_node::~dummy_node()
{
}

dummy_node& dummy_node::get(const std::string name)
{
  node* e = head_;
  while(e) {
    if(name == e->name())
      return * (dummy_node*) e;
    e = e->next();
  }
  return *(new dummy_node(name));
}

void dummy_node::info(std::ostream&)
{
}

void dummy_node::perlify(FILE* f) 
{
}

// template<> void ecf_concrete_node<dummy_node>::set_graphic_ptr(node* n){}

