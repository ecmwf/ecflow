#ifndef result_H
#define result_H
//=============================================================================================
// Name        : 
// Author      : 
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
//=============================================================================================


#include "node_alert.h"

class node;

class result : public node_alert<result> {
public:

	result();
	~result(); // Change to virtual if base class

private:

	result(const result&);
	result& operator=(const result&);

	virtual bool keep(node*);
};

inline void destroy(result**) {}
#endif
