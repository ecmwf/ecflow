#ifndef runnable_H
#define runnable_H
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


#include <Xm/Xm.h>

#ifdef NO_BOOL
#include "bool.h"
#endif

#ifndef extent_H
#include "extent.h"
#endif

class runnable : public extent<runnable> {
public:
	runnable();

	virtual ~runnable(); // Change to virtual if base class

	void enable();
	void disable();
	bool actived() { return actived_; }

	virtual void run() = 0;

private:
	runnable(const runnable&);
	runnable& operator=(const runnable&);

	Boolean actived_;

	static Boolean workCB(XtPointer);
};
#endif
