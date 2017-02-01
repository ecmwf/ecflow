#ifndef tip_H
#define tip_H
//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #5 $ 
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


#ifndef timeout_H
#include "timeout.h"
#endif

#include "uitip.h"

class tip : public timeout, public tip_shell_c {
public:

	tip(Widget);

	~tip(); // Change to virtual if base class

	static void makeTips(Widget);

private:

	tip(const tip&);
	tip& operator=(const tip&);

	Boolean in_;
	Widget  widget_;

	void enter();
	void leave();
	void run();
	
	static void enterCB(Widget,XtPointer,XEvent*,Boolean*);
	static void leaveCB(Widget,XtPointer,XEvent*,Boolean*);
};

inline void destroy(tip**) {}

#endif
