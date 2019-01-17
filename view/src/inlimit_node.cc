//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #10 $ 
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

#include "inlimit_node.h"
#include "ecf_node.h"
#include "NodeAttr.hpp"

inlimit_node::inlimit_node(host& h,ecf_node* n) 
  : node(h,n)
  , buf_()
 {
   if (owner_) buf_ = owner_->toString();
   full_name_ = parent()->full_name();
   full_name_ += ":";
   full_name_ += buf_;
}

#ifdef BRIDGE
extern "C" {
#define new _new
#define delete _delete
#include "smsproto.h"
}
inlimit_node::inlimit_node(host& h,sms_node* n, char b) 
  : node(h,n,b) 
  , buf_("limited by: ")
  , full_name_ ("inlimit: ")
{
  if (n) { buf_ += n->name;
    full_name_ = sms_node_full_name(n); 
  }
}
#endif

inlimit_node::~inlimit_node() {
}

xmstring inlimit_node::make_label_tree()
{
  char buf[1024];
  sprintf(buf,"%s",buf_.c_str());
  return xmstring(buf);
}

void inlimit_node::perlify(FILE* f) 
{
  perl_member(f,"limit",owner_->name().c_str());
  // perl_member(f,"usage",owner_->usage());
}

bool inlimit_node::match(const char* p)
{
  return strstr(owner_->name().c_str(), p) != 0;
}

const std::string& inlimit_node::full_name() const
{
  return full_name_;
}
