#ifndef observable_H
#define observable_H
//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #4 $ 
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


#ifdef NO_BOOL
#include "bool.h"
#endif

class observable  {
public:

	observable();

	virtual ~observable(); // Change to virtual if base class

	void notify_observers();
	void notify_adoption(observable*);

private:

	observable(const observable&);
	observable& operator=(const observable&);

	bool observed_;

	friend class relation;
};

/* #include "observable.cc" */

#endif
