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

#include "restart.h"
#include "node.h"


restart::restart():
	node_alert<restart>("Restarted tasks",STATUS_SUBMITTED)
{
}

restart::~restart()
{
}

bool restart::keep(node* n)
{
	return n->isRerun();
}
