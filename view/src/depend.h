#ifndef depend_H
#define depend_H
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


// Headers
// #ifndef   machine_H
// #include <machine.h>
// #endif

// Forward declarations

// class ostream;
// typedef class _Pvts os_typespec; // Remove if not persistant

class host;
// 

#include "uidepend.h"


class depend : public depend_shell_c {
public:

// -- Exceptions
	// None

// -- Contructors

	depend();

// -- Destructor

	~depend(); // Change to virtual if base class

// -- Convertors
	// None

// -- Operators

	void make(Widget);
	void raise(Widget);
	void hide();

// -- Methods


// -- Overridden methods
	// None

// -- Class members
	// None

// -- Class methods


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

	depend(const depend&);
	depend& operator=(const depend&);

// -- Members


// -- Methods



// -- Overridden methods


// -- Class members


// -- Class methods

	void closeCB(Widget,XtPointer);

// -- Friends

	//friend ostream& operator<<(ostream& s,const depend& p)
	//	{ p.print(s); return s; }

};

inline void destroy(depend**) {}

// If persistent, uncomment, otherwise remove
//#ifdef _ODI_OSSG_
//OS_MARK_SCHEMA_TYPE(depend);
//#endif

#endif
