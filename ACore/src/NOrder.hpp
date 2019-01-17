#ifndef NORDER_HPP_
#define NORDER_HPP_

//============================================================================
// Name        :
// Author      : Avi
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
//============================================================================

#include <string>

class NOrder {
	NOrder() {}
public:
	enum Order { TOP, BOTTOM, ALPHA, ORDER, UP, DOWN };

	static std::string toString(NOrder::Order);
	static NOrder::Order toOrder(const std::string&);
	static bool isValid(const std::string& order);
};

#endif
