//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #3 $ 
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


#include "not_enqueued.h"
#include "node.h"


not_enqueued::not_enqueued(): node_alert<not_enqueued>("Not_Enqueued tasks")
{
}

not_enqueued::~not_enqueued()
{
}

bool not_enqueued::keep(node* n)
{
  return false; // return n->isNotEnqueued();
}
