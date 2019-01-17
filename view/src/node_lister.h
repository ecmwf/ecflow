#ifndef node_lister_H
#define node_lister_H
//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #4 $ 
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


#ifndef observer_H
#include "observer.h"
#endif

#include <string>
class node;

class node_lister {
public:

	node_lister() {}

	virtual ~node_lister() {}

	virtual void next(node&) = 0;
	virtual void next(const std::string) {}
private:
	node_lister(const node_lister&);
	node_lister& operator=(const node_lister&);
};

inline void destroy(node_lister**) {}
#endif
