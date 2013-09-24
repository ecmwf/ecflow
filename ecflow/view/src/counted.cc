#ifndef counted_H
#include "counted.h"
#endif

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

counted::counted():
	count_(0)
{
}

counted::~counted()
{
}

void counted::attach()
{
	count_++;
}

void counted::detach()
{
	if(--count_ == 0) delete this;
}
