#ifndef input_H
#define input_H
//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #4 $ 
//
// Copyright 2009-2016 ECMWF. 
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description : 
//=============================================================================================


#include <Xm/Xm.h>

// Headers
// #ifndef   machine_H
// #include <machine.h>
// #endif

// Forward declarations

// class ostream;
// typedef class _Pvts os_typespec; // Remove if not persistant


// 

#include <stdio.h>

class input {
public:

// -- Exceptions
	// None

// -- Contructors

	input();

// -- Destructor

	virtual ~input(); // Change to virtual if base class

// -- Convertors
	// None

// -- Operators
	// None

// -- Methods
	// None

	void start(FILE*);
	void stop();

	virtual void ready(const char*) = 0;
	virtual void done(FILE*)        = 0;

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

	input(const input&);
	input& operator=(const input&);

// -- Members

	XtInputId  id_;
	FILE*       file_;

// -- Methods
	// None

// -- Overridden methods
	// None

// -- Class members

// -- Class methods
	// None
	static void inputCB(XtPointer,int*,XtInputId*);

// -- Friends

	//friend ostream& operator<<(ostream& s,const input& p)
	//	{ p.print(s); return s; }

};


#endif
