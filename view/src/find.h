#ifndef find_H
#define find_H
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

class host;

#include "uifind.h"

class runnable;

class find : public find_shell_c {
public:
	find();

	~find(); // Change to virtual if base class

	void make(Widget);
	void raise(Widget);
	void hide();
	void message(const char*,...);
	void no_message();
	virtual void search(const char*,bool,bool,bool,bool) = 0;

	void pending(runnable* r) { pending_ = r; }

private:

	find(const find&);
	find& operator=(const find&);

	runnable* pending_;

	void closeCB(Widget,XtPointer);
	void findCB(Widget,XtPointer);
	void regexCB(Widget,XtPointer);
	void entryCB(Widget,XtPointer);
};

inline void destroy(find**) {}
#endif
