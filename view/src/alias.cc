//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #7 $ 
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

#include "task_node.h"

#ifdef BRIDGE
alias_node::alias_node(host& h,sms_node* n,char b):
  task_node(h,n,b)
{}
#endif

alias_node::alias_node(host& h,ecf_node* n):
	task_node(h,n)
{
}

alias_node::~alias_node()
{
}


void alias_node::why(std::ostream& f)
{
	task_node::why(f);	
}

