#ifndef opener_H
#define opener_H
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


// Headers
// #ifndef   machine_H
// #include <machine.h>
// #endif

// Forward declarations

// class ostream;
// typedef class _Pvts os_typespec; // Remove if not persistant

// 

#include <Xm/Xm.h>
#include "runnable.h"

class opener : public runnable {
public:

// -- Exceptions
	// None

// -- Contructors

	opener() : widget_(0) {}

// -- Destructor


// -- Convertors
	// None

// -- Operators
	// None

// -- Methods

	void show(Widget w) { widget_ = w; enable(); }

// -- Overridden methods
	// None

// -- Class members
	// None

// -- Class methods
	// None

	// Uncomment for persistent, remove otherwise
	// static os_typespec* get_os_typespec();

protected:

// -- Members
	// None

// -- Methods
	
	// void print(ostream&) const; // Change to virtual if base class	

// -- Overridden methods
	// None

// -- Class members
	// None

// -- Class methods
	// None

private:

// No copy allowed

	opener(const opener&);
	opener& operator=(const opener&);

// -- Members

	Widget widget_;

// -- Methods
	// None

// -- Overridden methods

	void run()  { if(!XtIsManaged(widget_)) XtManageChild(widget_); disable();}


// -- Class members
	// None

// -- Class methods
	// None

// -- Friends

	//friend ostream& operator<<(ostream& s,const opener& p)
	//	{ p.print(s); return s; }

};

inline void destroy(opener**) {}

// If persistent, uncomment, otherwise remove
//#ifdef _ODI_OSSG_
//OS_MARK_SCHEMA_TYPE(opener);
//#endif

#endif
