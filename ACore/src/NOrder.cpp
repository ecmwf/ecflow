//============================================================================
// Name        :
// Author      : Avi
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
//============================================================================

#include "NOrder.hpp"
#include <assert.h>

std::string NOrder::toString(NOrder::Order s) {
	switch (s) {
		case NOrder::TOP:     return "top"; break;
		case NOrder::BOTTOM:  return "bottom"; break;
 		case NOrder::ALPHA:   return "alpha"; break;
		case NOrder::ORDER:   return "order"; break;
		case NOrder::UP:      return "up"; break;
		case NOrder::DOWN:    return "down"; break;
		default: assert(false); break;
	}
	assert(false);
	return std::string();
}

NOrder::Order NOrder::toOrder(const std::string& str)
{
	if (str == "top")     return NOrder::TOP;
	if (str == "bottom")  return NOrder::BOTTOM;
	if (str == "alpha")   return NOrder::ALPHA;
	if (str == "order")   return NOrder::ORDER;
	if (str == "up")      return NOrder::UP;
	if (str == "down")    return NOrder::DOWN;
 	assert(false);
	return NOrder::TOP;
}

bool NOrder::isValid(const std::string& order)
{
	if (order == "top")  return true;
	if (order == "bottom")   return true;
	if (order == "alpha")    return true;
	if (order == "order")   return true;
	if (order == "up") return true;
	if (order == "down") return true;
 	return false;
}

