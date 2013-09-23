#ifndef timeout_H
#define timeout_H

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

#ifndef extent_H
#include "extent.h"
#endif

class timeout : public extent<timeout> {
public:

	timeout(double);

	virtual ~timeout(); // Change to virtual if base class

	void enable();
	void disable();
	void frequency(double);
	void drift(double,double);

	virtual void run() = 0;

private:

	timeout(const timeout&);
	timeout& operator=(const timeout&);

	Boolean actived_;
	double frequency_;
	XtIntervalId id_;
	int running_;

	static void timeoutCB(XtPointer,XtIntervalId*);

};

#endif
