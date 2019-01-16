//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #3 $ 
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

#include "to_check.h"
#include "zombie.h"

to_check::to_check() : node_alert<to_check>("Tasks to check") {}
to_check::~to_check() {} // Change to virtual if base class
bool to_check::keep(node* n) { return n->isToBeChecked(); }

zombie::zombie() : node_alert<zombie>("Zombies") {}
zombie::~zombie() {} // Change to virtual if base class
bool zombie::keep(node* n) { return n->isZombie(); }

